/*
Copyright (C) 2017, 2018 Topological Manifold

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
По книге

Eleanor Chu, Alan George.
INSIDE the FFT BLACK BOX. Serial and Parallel Fast Fourier Transform Algorithms.
CRC Press LLC, 2000.

Chapter 13: FFTs for Arbitrary N.

В этой книге в главе 13 есть ошибки при вычислении H2

  В примере 13.4.
    Написано:
      h0, h1, h2, h3, h4, h5, 0, 0, 0, 0, 0,  0, h4, h3, h2, h1.
    Надо:
      h0, h1, h2, h3, h4, h5, 0, 0, 0, 0, 0, h5, h4, h3, h2, h1.

  В формулах 13.11, 13.23, 13.24, 13.25.
    Написано:
      h2(l) = h(l) для l = 0,...,N - 1,
      h2(l) = 0 для l = N,..., M - N + 1,
      h2(l) = h(M - l) для l = M - N + 2,..., M - 1.
    Надо:
      h2(l) = h(l) для l = 0,...,N - 1,
      h2(l) = 0 для l = N,..., M - N,
      h2(l) = h(M - l) для l = M - N + 1,..., M - 1.
*/

#include "dft_gl2d.h"

#include "memory.h"
#include "program.h"

#include "com/bits.h"
#include "com/error.h"
#include "com/log.h"
#include "com/math.h"
#include "com/print.h"
#include "com/time.h"
#include "gpgpu/com/groups.h"
#include "graphics/opengl/query.h"

#include <complex>
#include <sstream>
#include <type_traits>
#include <vector>

constexpr const int GROUP_SIZE_1D = 256;
constexpr const vec2i GROUP_SIZE_2D = vec2i(16, 16);

namespace
{
// Или само число степень двух,
// или минимальная степень двух, равная или больше 2N-2
int compute_M(int n)
{
        int log2_N = log_2(n);
        if ((1 << log2_N) == n)
        {
                return n;
        }

        int t = (2 * n - 2);
        int log2_t = log_2(t);
        if ((1 << log2_t) == t)
        {
                return t;
        }
        else
        {
                return (1 << log2_t) << 1;
        }
}

// Compute the symmetric Toeplitz H: for given N, compute the scalar constants
// Формулы 13.4, 13.22.
std::vector<std::complex<double>> compute_h(int N, bool inverse, double Coef)
{
        std::vector<std::complex<double>> h(N);

        for (int l = 0; l <= N - 1; ++l)
        {
                // theta = (inverse ? 1 : -1) * 2 * pi / N * (-0.5 * l * l) = (inverse ? -pi : pi) / N * l * l

                // h[l] = std::polar(Coef, (inverse ? -PI : PI) / N * l * l);

                // Вместо l * l / N нужно вычислить mod(l * l / N, 2), чтобы в тригонометрические функции
                // поступало не больше 2 * PI.
                long long dividend = l * l;
                long long quotient = dividend / N;
                long long remainder = dividend - quotient * N;
                // factor = (quotient mod 2) + (remainder / N).
                double factor = (quotient & 1) + static_cast<double>(remainder) / N;

                h[l] = std::polar(Coef, (inverse ? -PI<double> : PI<double>)*factor);
        }

        return h;
}

// Embed H in the circulant H(2)
// На основе исправленных формул 13.11, 13.23, 13.24, 13.25.
// Об исправлении в комментарии о книге.
std::vector<std::complex<double>> compute_h2(int N, int M, const std::vector<std::complex<double>>& h)
{
        std::vector<std::complex<double>> h2(M);

        for (int l = 0; l <= N - 1; ++l)
        {
                h2[l] = h[l];
        }
        for (int l = N; l <= M - N; ++l)
        {
                h2[l] = std::complex<double>(0, 0);
        }
        for (int l = M - N + 1; l <= M - 1; ++l)
        {
                h2[l] = h[M - l];
        }
        return h2;
}

template <typename Dst, typename Src>
std::vector<std::complex<Dst>> conv(const std::vector<std::complex<Src>>& data)
{
        if constexpr (std::is_same_v<Dst, Src>)
        {
                return data;
        }
        else
        {
                std::vector<std::complex<Dst>> res(data.size());
                for (size_t i = 0; i < data.size(); ++i)
                {
                        res[i] = {static_cast<Dst>(data[i].real()), static_cast<Dst>(data[i].imag())};
                }
                return res;
        }
}

template <typename Dst, typename Src>
std::enable_if_t<std::is_same_v<Dst, Src>, std::vector<std::complex<Dst>>&&> conv(std::vector<std::complex<Src>>&& data)
{
        return std::move(data);
}

template <typename FP>
int shared_size(int dft_size)
{
        // минимум из
        // 1) требуемый размер, но не меньше 128, чтобы в группе было хотя бы 64 потока по потоку на 2 элемента:
        //   NVIDIA работает по 32 потока вместе (warp), AMD по 64 потока вместе (wavefront).
        // 2) максимальная степень 2, которая меньше или равна вместимости разделяемой памяти
        return std::min(std::max(128, dft_size), 1 << log_2(opengl::max_compute_shared_memory() / sizeof(std::complex<FP>)));
}

template <typename FP>
int group_size(int dft_size)
{
        // не больше 1 потока на 2 элемента
        int max_threads_required = shared_size<FP>(dft_size) / 2;
        int max_threads_supported = std::min(opengl::max_fixed_group_size_x(), opengl::max_fixed_group_invocations());
        return std::min(max_threads_required, max_threads_supported);
}

template <typename FP>
void fft1d(bool inverse, int fft_count, const DeviceProgFFTShared<FP>& fft, const DeviceProg<FP>& programs,
           DeviceMemory<std::complex<FP>>* data)
{
        const int N = fft.n();

        if (N == 1)
        {
                return;
        }

        const int shared_size = fft.shared_size();
        const int data_size = N * fft_count;

        if (N <= shared_size)
        {
                fft.exec(inverse, data_size, data);
                return;
        }

        const int N_bits = fft.n_bits();
        ASSERT((1 << N_bits) == N);

        // Если N превышает максимум обрабатываемых данных shared_size, то вначале
        // надо отдельно выполнить перестановку данных, а потом запускать функции
        // с отключенной перестановкой, иначе одни запуски будут вносить изменения
        // в данные других запусков, так как результат пишется в исходные данные.

        programs.bit_reverse(data_size, N - 1, N_bits, data);

        fft.exec(inverse, data_size, data);

        // Досчитать до нужного размера уже в глобальной памяти без разделяемой

        const int N_2 = N / 2;
        const int N_2_mask = N_2 - 1;
        const int N_2_bits = N_bits - 1;

        const int thread_count = data_size / 2;

        int M_2 = shared_size;
        FP Two_PI_Div_M = inverse ? (PI<FP> / M_2) : -(PI<FP> / M_2);

        for (; M_2 < N; M_2 <<= 1, Two_PI_Div_M /= 2)
        {
                // M_2 - половина размера текущих отдельных БПФ.
                programs.fft(thread_count, inverse, Two_PI_Div_M, N_2_mask, N_2_bits, M_2, data);
        }
}

template <typename FP>
class Impl final : public FourierGL1, public FourierGL2
{
        const int m_N1, m_N2, m_M1, m_M2, m_M1_bin, m_M2_bin;
        DeviceMemory<std::complex<FP>> m_d1_fwd, m_d1_inv, m_d2_fwd, m_d2_inv;
        DeviceMemory<std::complex<FP>> m_x_d, m_buffer;
        GLuint64 m_texture_handle;
        DeviceProg<FP> m_prog;
        DeviceProgCopy<FP> m_copy;
        DeviceProgMul<FP> m_mul;
        DeviceProgMulD<FP> m_mul_d;
        DeviceProgFFTShared<FP> m_fft_1;
        DeviceProgFFTShared<FP> m_fft_2;

        void dft2d(bool inverse)
        {
                if (m_N1 > 1)
                {
                        // По строкам

                        m_mul.rows_to_buffer(inverse, m_x_d, &m_buffer);
                        fft1d(inverse, m_N2, m_fft_1, m_prog, &m_buffer);
                        m_mul_d.rows_mul_d(inverse ? m_d1_inv : m_d1_fwd, &m_buffer);
                        fft1d(!inverse, m_N2, m_fft_1, m_prog, &m_buffer);
                        m_mul.rows_from_buffer(inverse, &m_x_d, m_buffer);
                }

                if (m_N2 > 1)
                {
                        // По столбцам

                        m_mul.columns_to_buffer(inverse, m_x_d, &m_buffer);
                        fft1d(inverse, m_N1, m_fft_2, m_prog, &m_buffer);
                        m_mul_d.columns_mul_d(inverse ? m_d2_inv : m_d2_fwd, &m_buffer);
                        fft1d(!inverse, m_N1, m_fft_2, m_prog, &m_buffer);
                        m_mul.columns_from_buffer(inverse, &m_x_d, m_buffer);
                }
        }

        void exec(bool inverse, std::vector<std::complex<float>>* src) override
        {
                int size = src->size();
                if (size != m_N1 * m_N2)
                {
                        error("FFT input size error: input " + to_string(size) + ", must be " + to_string(m_N1 * m_N2));
                }

                std::vector<std::complex<FP>> data = conv<FP>(std::move(*src));

                m_x_d.load(data);

                glFinish();

                double start_time = time_in_seconds();

                dft2d(inverse);

                glFinish();

                LOG("calc gl2d: " + to_string_fixed(1000.0 * (time_in_seconds() - start_time), 5) + " ms");

                m_x_d.read(&data);

                *src = conv<float>(std::move(data));
        }

        void exec(bool inverse, bool srgb) override
        {
                m_copy.copy_input(srgb, m_texture_handle, &m_x_d);
                dft2d(inverse);
                m_copy.copy_output(static_cast<FP>(1.0 / (m_N1 * m_N2)), m_texture_handle, m_x_d);
        }

public:
        Impl(int n1, int n2, const opengl::TextureRGBA32F* texture)
                : m_N1(n1),
                  m_N2(n2),
                  m_M1(compute_M(m_N1)),
                  m_M2(compute_M(m_N2)),
                  m_M1_bin(binary_size(m_M1)),
                  m_M2_bin(binary_size(m_M2)),
                  m_d1_fwd(m_M1, MemoryUsage::StaticCopy),
                  m_d1_inv(m_M1, MemoryUsage::StaticCopy),
                  m_d2_fwd(m_M2, MemoryUsage::StaticCopy),
                  m_d2_inv(m_M2, MemoryUsage::StaticCopy),
                  m_x_d(m_N1 * m_N2, MemoryUsage::DynamicCopy),
                  m_buffer(std::max(m_M1 * m_N2, m_M2 * m_N1), MemoryUsage::DynamicCopy),
                  m_prog(GROUP_SIZE_1D),
                  m_copy(GROUP_SIZE_2D, m_N1, m_N2),
                  m_mul(GROUP_SIZE_2D, m_N1, m_N2, m_M1, m_M2),
                  m_mul_d(GROUP_SIZE_2D, m_N1, m_N2, m_M1, m_M2),
                  m_fft_1(m_M1, shared_size<FP>(m_M1), group_size<FP>(m_M1), m_M1 <= shared_size<FP>(m_M1)),
                  m_fft_2(m_M2, shared_size<FP>(m_M2), group_size<FP>(m_M2), m_M2 <= shared_size<FP>(m_M2))

        {
                if (m_N1 < 1 || m_N2 < 1)
                {
                        error("FFT size error: " + to_string(m_N1) + "x" + to_string(m_N2));
                }

                if (texture)
                {
                        ASSERT(texture->texture().width() == n1 && texture->texture().height() == n2);

                        m_texture_handle = texture->image_resident_handle_read_write();
                }

                // Для обратного преобразования нужна корректировка данных с умножением на коэффициент,
                // так как разный размер у исходного вектора N и его расширенного M.
                double M1_Div_N1 = static_cast<double>(m_M1) / m_N1;
                double M2_Div_N2 = static_cast<double>(m_M2) / m_N2;

                // Compute the diagonal D in Lemma 13.2: use the radix-2 FFT
                // Формулы 13.13, 13.26.

                m_d1_fwd.load(conv<FP>(compute_h2(m_N1, m_M1, compute_h(m_N1, false, 1.0))));
                fft1d(false, 1, m_fft_1, m_prog, &m_d1_fwd);

                m_d1_inv.load(conv<FP>(compute_h2(m_N1, m_M1, compute_h(m_N1, true, M1_Div_N1))));
                fft1d(true, 1, m_fft_1, m_prog, &m_d1_inv);

                m_d2_fwd.load(conv<FP>(compute_h2(m_N2, m_M2, compute_h(m_N2, false, 1.0))));
                fft1d(false, 1, m_fft_2, m_prog, &m_d2_fwd);

                m_d2_inv.load(conv<FP>(compute_h2(m_N2, m_M2, compute_h(m_N2, true, M2_Div_N2))));
                fft1d(true, 1, m_fft_2, m_prog, &m_d2_inv);
        }
};
}

std::unique_ptr<FourierGL1> create_dft_gl2d(int x, int y)
{
        return std::make_unique<Impl<float>>(x, y, nullptr);
}

std::unique_ptr<FourierGL2> create_dft_gl2d(int x, int y, const opengl::TextureRGBA32F& texture)
{
        return std::make_unique<Impl<float>>(x, y, &texture);
}
