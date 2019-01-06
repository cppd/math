/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "dft_compute.h"

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
int compute_m(int n)
{
        int log2_n = log_2(n);
        if ((1 << log2_n) == n)
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
std::vector<std::complex<double>> compute_h(int n, bool inverse, double coef)
{
        std::vector<std::complex<double>> h(n);

        for (int l = 0; l <= n - 1; ++l)
        {
                // theta = (inverse ? 1 : -1) * 2 * pi / n * (-0.5 * l * l) = (inverse ? -pi : pi) / n * l * l

                // h[l] = std::polar(Coef, (inverse ? -PI : PI) / n * l * l);

                // Вместо l * l / n нужно вычислить mod(l * l / n, 2), чтобы в тригонометрические функции
                // поступало не больше 2 * PI.
                long long dividend = l * l;
                long long quotient = dividend / n;
                long long remainder = dividend - quotient * n;
                // factor = (quotient mod 2) + (remainder / n).
                double factor = (quotient & 1) + static_cast<double>(remainder) / n;

                h[l] = std::polar(coef, (inverse ? -PI<double> : PI<double>)*factor);
        }

        return h;
}

// Embed H in the circulant H(2)
// На основе исправленных формул 13.11, 13.23, 13.24, 13.25.
// Об исправлении в комментарии о книге.
std::vector<std::complex<double>> compute_h2(int n, int m, const std::vector<std::complex<double>>& h)
{
        std::vector<std::complex<double>> h2(m);

        for (int l = 0; l <= n - 1; ++l)
        {
                h2[l] = h[l];
        }
        for (int l = n; l <= m - n; ++l)
        {
                h2[l] = std::complex<double>(0, 0);
        }
        for (int l = m - n + 1; l <= m - 1; ++l)
        {
                h2[l] = h[m - l];
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
void fft1d(bool inverse, int fft_count, const DeviceProgFFTShared<FP>& fft, const DeviceProgBitReverse<FP>& bit_reverse,
           const DeviceProgFFTGlobal<FP>& fft_global, DeviceMemory<std::complex<FP>>* data)
{
        const int n = fft.n();

        if (n == 1)
        {
                return;
        }

        const int shared_size = fft.shared_size();
        const int data_size = n * fft_count;

        if (n <= shared_size)
        {
                fft.exec(inverse, data_size, *data);
                return;
        }

        const int n_bits = fft.n_bits();
        ASSERT((1 << n_bits) == n);

        // Если n превышает максимум обрабатываемых данных shared_size, то вначале
        // надо отдельно выполнить перестановку данных, а потом запускать функции
        // с отключенной перестановкой, иначе одни запуски будут вносить изменения
        // в данные других запусков, так как результат пишется в исходные данные.

        bit_reverse.exec(data_size, n - 1, n_bits, *data);

        fft.exec(inverse, data_size, *data);

        // Досчитать до нужного размера уже в глобальной памяти без разделяемой

        const int n_div_2 = n / 2;
        const int n_div_2_mask = n_div_2 - 1;

        const int thread_count = data_size / 2;

        int m_div_2 = shared_size;
        FP two_pi_div_m = inverse ? (PI<FP> / m_div_2) : -(PI<FP> / m_div_2);

        for (; m_div_2 < n; m_div_2 <<= 1, two_pi_div_m /= 2)
        {
                // m_div_2 - половина размера текущих отдельных БПФ
                fft_global.exec(thread_count, inverse, two_pi_div_m, n_div_2_mask, m_div_2, *data);
        }
}

template <typename FP>
class Impl final : public DFTCompute, public DFTComputeTexture
{
        const int m_n1, m_n2, m_m1, m_m2, m_m1_bin, m_m2_bin;
        DeviceMemory<std::complex<FP>> m_d1_fwd, m_d1_inv, m_d2_fwd, m_d2_inv;
        DeviceMemory<std::complex<FP>> m_x_d, m_buffer;
        GLuint64 m_texture_handle;
        DeviceProgBitReverse<FP> m_bit_reverse;
        DeviceProgFFTGlobal<FP> m_fft_global;
        DeviceProgCopyInput<FP> m_copy_input;
        DeviceProgCopyOutput<FP> m_copy_output;
        DeviceProgMul<FP> m_mul;
        DeviceProgMulD<FP> m_mul_d;
        DeviceProgFFTShared<FP> m_fft_1;
        DeviceProgFFTShared<FP> m_fft_2;

        void dft2d(bool inverse)
        {
                if (m_n1 > 1)
                {
                        // По строкам

                        m_mul.rows_to_buffer(inverse, m_x_d, m_buffer);
                        fft1d(inverse, m_n2, m_fft_1, m_bit_reverse, m_fft_global, &m_buffer);
                        m_mul_d.rows_mul_d(inverse ? m_d1_inv : m_d1_fwd, m_buffer);
                        fft1d(!inverse, m_n2, m_fft_1, m_bit_reverse, m_fft_global, &m_buffer);
                        m_mul.rows_from_buffer(inverse, m_x_d, m_buffer);
                }

                if (m_n2 > 1)
                {
                        // По столбцам

                        m_mul.columns_to_buffer(inverse, m_x_d, m_buffer);
                        fft1d(inverse, m_n1, m_fft_2, m_bit_reverse, m_fft_global, &m_buffer);
                        m_mul_d.columns_mul_d(inverse ? m_d2_inv : m_d2_fwd, m_buffer);
                        fft1d(!inverse, m_n1, m_fft_2, m_bit_reverse, m_fft_global, &m_buffer);
                        m_mul.columns_from_buffer(inverse, m_x_d, m_buffer);
                }
        }

        void exec(bool inverse, std::vector<std::complex<float>>* src) override
        {
                int size = src->size();
                if (size != m_n1 * m_n2)
                {
                        error("FFT input size error: input " + to_string(size) + ", must be " + to_string(m_n1 * m_n2));
                }

                std::vector<std::complex<FP>> data = conv<FP>(std::move(*src));

                m_x_d.write(data);

                glFinish();

                double start_time = time_in_seconds();

                dft2d(inverse);

                glFinish();

                LOG("calc OpenGL: " + to_string_fixed(1000.0 * (time_in_seconds() - start_time), 5) + " ms");

                m_x_d.read(&data);

                *src = conv<float>(std::move(data));
        }

        void exec(bool inverse, bool srgb) override
        {
                m_copy_input.copy(srgb, m_texture_handle, m_x_d);
                dft2d(inverse);
                m_copy_output.copy(static_cast<FP>(1.0 / (m_n1 * m_n2)), m_texture_handle, m_x_d);
        }

public:
        Impl(int n1, int n2, const opengl::TextureRGBA32F* texture)
                : m_n1(n1),
                  m_n2(n2),
                  m_m1(compute_m(m_n1)),
                  m_m2(compute_m(m_n2)),
                  m_m1_bin(binary_size(m_m1)),
                  m_m2_bin(binary_size(m_m2)),
                  m_d1_fwd(m_m1),
                  m_d1_inv(m_m1),
                  m_d2_fwd(m_m2),
                  m_d2_inv(m_m2),
                  m_x_d(m_n1 * m_n2),
                  m_buffer(std::max(m_m1 * m_n2, m_m2 * m_n1)),
                  m_bit_reverse(GROUP_SIZE_1D),
                  m_fft_global(GROUP_SIZE_1D),
                  m_copy_input(GROUP_SIZE_2D, m_n1, m_n2),
                  m_copy_output(GROUP_SIZE_2D, m_n1, m_n2),
                  m_mul(GROUP_SIZE_2D, m_n1, m_n2, m_m1, m_m2),
                  m_mul_d(GROUP_SIZE_2D, m_n1, m_n2, m_m1, m_m2),
                  m_fft_1(m_m1, shared_size<FP>(m_m1), group_size<FP>(m_m1), m_m1 <= shared_size<FP>(m_m1)),
                  m_fft_2(m_m2, shared_size<FP>(m_m2), group_size<FP>(m_m2), m_m2 <= shared_size<FP>(m_m2))

        {
                if (m_n1 < 1 || m_n2 < 1)
                {
                        error("FFT size error: " + to_string(m_n1) + "x" + to_string(m_n2));
                }

                if (texture)
                {
                        ASSERT(texture->texture().width() == n1 && texture->texture().height() == n2);

                        m_texture_handle = texture->image_resident_handle_read_write();
                }

                // Для обратного преобразования нужна корректировка данных с умножением на коэффициент,
                // так как разный размер у исходного вектора N и его расширенного M.
                double m1_div_n1 = static_cast<double>(m_m1) / m_n1;
                double m2_div_n2 = static_cast<double>(m_m2) / m_n2;

                // Compute the diagonal D in Lemma 13.2: use the radix-2 FFT
                // Формулы 13.13, 13.26.

                m_d1_fwd.write(conv<FP>(compute_h2(m_n1, m_m1, compute_h(m_n1, false, 1.0))));
                fft1d(false, 1, m_fft_1, m_bit_reverse, m_fft_global, &m_d1_fwd);

                m_d1_inv.write(conv<FP>(compute_h2(m_n1, m_m1, compute_h(m_n1, true, m1_div_n1))));
                fft1d(true, 1, m_fft_1, m_bit_reverse, m_fft_global, &m_d1_inv);

                m_d2_fwd.write(conv<FP>(compute_h2(m_n2, m_m2, compute_h(m_n2, false, 1.0))));
                fft1d(false, 1, m_fft_2, m_bit_reverse, m_fft_global, &m_d2_fwd);

                m_d2_inv.write(conv<FP>(compute_h2(m_n2, m_m2, compute_h(m_n2, true, m2_div_n2))));
                fft1d(true, 1, m_fft_2, m_bit_reverse, m_fft_global, &m_d2_inv);
        }
};
}

std::unique_ptr<DFTCompute> create_dft_compute(int x, int y)
{
        return std::make_unique<Impl<float>>(x, y, nullptr);
}

std::unique_ptr<DFTComputeTexture> create_dft_compute_texture(int x, int y, const opengl::TextureRGBA32F& texture)
{
        return std::make_unique<Impl<float>>(x, y, &texture);
}
