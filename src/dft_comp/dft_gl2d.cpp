/*
Copyright (C) 2017 Topological Manifold

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

#include "device_mem.h"
#include "device_prog.h"

#include "com/bits.h"
#include "com/error.h"
#include "com/log.h"
#include "com/math.h"
#include "com/print.h"
#include "gl/gl_query.h"

#include <chrono>
#include <complex>
#include <sstream>
#include <vector>

namespace
{
template <typename T>
void print(const DeviceMemory<T>& dm)
{
        std::vector<T> tmp(dm.size());
        dm.read(&tmp);
        print_vector(tmp);
}

// Или само число степень двух,
// или минимальная степень двух, равная или больше 2N-2
int compute_M(int n)
{
        int log2_N = get_log_2(n);
        if ((1 << log2_N) == n)
        {
                return n;
        }

        int t = (2 * n - 2);
        int log2_t = get_log_2(t);
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
std::vector<std::complex<double>> compute_h(int N, bool inv, double Coef)
{
        std::vector<std::complex<double>> h(N);

        for (int l = 0; l <= N - 1; ++l)
        {
                // theta = (inv ? 1 : -1) * 2 * pi / N * (-0.5 * l * l) = (inv ? -pi : pi) / N * l * l

                // h[l] = std::polar(Coef, (inv ? -PI : PI) / N * l * l);

                // Вместо l * l / N нужно вычислить mod(l * l / N, 2), чтобы в тригонометрические функции
                // поступало не больше 2 * PI.
                long long dividend = l * l;
                long long quotient = dividend / N;
                long long remainder = dividend - quotient * N;
                // factor = (quotient mod 2) + (remainder / N).
                double factor = (quotient & 1) + static_cast<double>(remainder) / N;

                h[l] = std::polar(Coef, (inv ? -PI : PI) * factor);
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

template <typename FP>
std::vector<std::complex<FP>> conv(const std::vector<std::complex<double>>& data)
{
        std::vector<std::complex<FP>> res(data.size());
        for (size_t i = 0; i < data.size(); ++i)
        {
                res[i] = std::complex<FP>(data[i].real(), data[i].imag());
        }
        return res;
}

template <typename FP>
int get_shared_size(int dft_size)
{
        // минимум из
        // 1) требуемый размер, но не меньше 128, чтобы в группе было хотя бы 64 потока по потоку на 2 элемента:
        //   NVIDIA работает по 32 потока вместе (warp), AMD по 64 потока вместе (wavefront).
        // 2) максимальная степень 2, которая меньше или равна вместимости разделяемой памяти
        return std::min(std::max(128, dft_size), 1 << get_log_2(get_max_compute_shared_memory() / sizeof(std::complex<FP>)));
}
template <typename FP>
int get_group_size(int dft_size)
{
        // не больше 1 потока на 2 элемента
        int max_threads_required = get_shared_size<FP>(dft_size) / 2;
        int max_threads_supported = std::min(get_max_work_group_size_x(), get_max_work_group_invocations());
        return std::min(max_threads_required, max_threads_supported);
}

template <typename FP>
class GL2D final : public IFourierGL1, public IFourierGL2
{
        const int BLOCK_SIZE = 256;
        const int BLOCK_SQRT = std::lround(std::sqrt(BLOCK_SIZE));
        const int m_N1, m_N2, m_M1, m_M2, m_M1_bin, m_M2_bin;
        const glm::ivec2 block, rows_to, rows_fr, rows_D, cols_to, cols_fr, cols_D;
        DeviceMemory<std::complex<FP>> m_D1_fwd, m_D1_inv, m_D2_fwd, m_D2_inv;
        DeviceMemory<std::complex<FP>> m_x_d, m_buffer;
        GLuint64 m_TextureHandle;
        const int m_shared_size_1, m_shared_size_2;
        const int m_group_size_1, m_group_size_2;
        DeviceProg<FP> m_prog;
        DeviceProgFFTRadix2<FP> m_FFT_1;
        DeviceProgFFTRadix2<FP> m_FFT_2;

        template <int type>
        void fft1d(bool inv, int rows, DeviceMemory<std::complex<FP>>* data) const
        {
                static_assert(type == 1 || type == 2, "fft1d calling error");

                const int N = (type == 1) ? m_M1 : m_M2;

                if (N == 1)
                {
                        return;
                }

                const int N_bits = (type == 1) ? m_M1_bin : m_M2_bin;
                const int data_size = N * rows;
                const int shared_size = (type == 1) ? m_shared_size_1 : m_shared_size_2;

                if (N <= shared_size)
                {
                        (type == 1) ? m_FFT_1.exec(inv, data_size, data) : m_FFT_2.exec(inv, data_size, data);
                }
                else
                {
                        // Если N превышает максимум обрабатываемых данных shared_size для функций
                        // m_prog.FFT_1 и m_prog.FFT_2, то вначале надо отдельно выполнить перестановку данных,
                        // а потом запускать функции с отключенной перестановкой, иначе одни запуски будут
                        // вносить изменения в данные других запусков, так как результат пишется в исходные данные.
                        m_prog.reverse(get_group_count(data_size, BLOCK_SIZE), BLOCK_SIZE, data_size, N - 1, N_bits, data);

                        (type == 1) ? m_FFT_1.exec(inv, data_size, data) : m_FFT_2.exec(inv, data_size, data);

                        // досчитать до нужного размера уже в глобальной памяти без разделяемой

                        const int N_2 = N / 2;
                        const int N_2_mask = N_2 - 1;
                        const int N_2_bits = N_bits - 1;

                        const int thread_cnt = data_size / 2;
                        const int block_cnt = get_group_count(thread_cnt, BLOCK_SIZE);

                        int M_2 = shared_size;
                        double Two_PI_Div_M = inv ? (PI / M_2) : -(PI / M_2);

                        for (; M_2 < N; M_2 <<= 1, Two_PI_Div_M *= 0.5)
                        {
                                // M_2 - половина размера текущих отдельных БПФ.
                                m_prog.FFT(block_cnt, BLOCK_SIZE, inv, thread_cnt, Two_PI_Div_M, N_2_mask, N_2_bits, M_2, data);
                        }
                }
        }

        void dft2d(bool inv)
        {
                if (m_N1 > 1)
                {
                        // по строкам

                        m_prog.rows_mul_to_buffer(rows_to, block, inv, m_M1, m_N1, m_N2, m_x_d, &m_buffer);
                        fft1d<1>(inv, m_N2, &m_buffer);
                        m_prog.rows_mul_D(rows_D, block, m_M1, m_N2, inv ? m_D1_inv : m_D1_fwd, &m_buffer);
                        fft1d<1>(!inv, m_N2, &m_buffer);
                        m_prog.rows_mul_fr_buffer(rows_fr, block, inv, m_M1, m_N1, m_N2, &m_x_d, m_buffer);
                }

                if (m_N2 > 1)
                {
                        // по столбцам

                        m_prog.cols_mul_to_buffer(cols_to, block, inv, m_M2, m_N1, m_N2, m_x_d, &m_buffer);
                        fft1d<2>(inv, m_N1, &m_buffer);
                        m_prog.rows_mul_D(cols_D, block, m_M2, m_N1, inv ? m_D2_inv : m_D2_fwd, &m_buffer);
                        fft1d<2>(!inv, m_N1, &m_buffer);
                        m_prog.cols_mul_fr_buffer(cols_fr, block, inv, m_M2, m_N1, m_N2, &m_x_d, m_buffer);
                }
        }

        void exec(bool inv, std::vector<std::complex<float>>* src) override
        {
                int size = src->size();
                if (size != m_N1 * m_N2)
                {
                        error("FFT input size error: input " + std::to_string(size) + ", must be " + std::to_string(m_N1 * m_N2));
                }

                std::vector<std::complex<FP>> data(src->size());
                for (size_t i = 0; i < src->size(); ++i)
                {
                        data[i] = std::complex<FP>(static_cast<FP>((*src)[i].real()), static_cast<FP>((*src)[i].imag()));
                }

                m_x_d.load(data);

                glFinish();
                std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();

                dft2d(inv);

                glFinish();
                std::chrono::duration<double, std::milli> time = std::chrono::steady_clock::now() - start;
                LOG("calc gl2d: " + to_string(time.count()));

                m_x_d.read(&data);

                for (size_t i = 0; i < src->size(); ++i)
                {
                        (*src)[i] = std::complex<float>(data[i].real(), data[i].imag());
                }
        }
        void exec(bool inv, bool srgb) override
        {
                glm::ivec2 grid(get_group_count(m_N1, block.x), get_group_count(m_N2, block.y));

                m_prog.move_to_input(grid, block, m_N1, m_N2, srgb, m_TextureHandle, &m_x_d);
                dft2d(inv);
                m_prog.move_to_output(grid, block, m_N1, m_N2, 1.0f / (m_N1 * m_N2), m_TextureHandle, m_x_d);
        }

public:
        GL2D(int n1, int n2, const TextureRGBA32F* tex)
                : m_N1(n1),
                  m_N2(n2),
                  m_M1(compute_M(m_N1)),
                  m_M2(compute_M(m_N2)),
                  m_M1_bin(get_bin_size(m_M1)),
                  m_M2_bin(get_bin_size(m_M2)),
                  block(BLOCK_SQRT, BLOCK_SQRT),
                  rows_to(get_group_count(m_M1, block.x), get_group_count(m_N2, block.y)),
                  rows_fr(get_group_count(m_N1, block.x), get_group_count(m_N2, block.y)),
                  rows_D(get_group_count(m_M1, block.x), get_group_count(m_N2, block.y)),
                  cols_to(get_group_count(m_N1, block.x), get_group_count(m_M2, block.y)),
                  cols_fr(get_group_count(m_N1, block.x), get_group_count(m_N2, block.y)),
                  cols_D(get_group_count(m_M2, block.x), get_group_count(m_N1, block.y)),
                  m_D1_fwd(m_M1, MemoryUsage::STATIC_COPY),
                  m_D1_inv(m_M1, MemoryUsage::STATIC_COPY),
                  m_D2_fwd(m_M2, MemoryUsage::STATIC_COPY),
                  m_D2_inv(m_M2, MemoryUsage::STATIC_COPY),
                  m_x_d(m_N1 * m_N2, MemoryUsage::DYNAMIC_COPY),
                  m_buffer(std::max(m_M1 * m_N2, m_M2 * m_N1), MemoryUsage::DYNAMIC_COPY),
                  m_shared_size_1(get_shared_size<FP>(m_M1)),
                  m_shared_size_2(get_shared_size<FP>(m_M2)),
                  m_group_size_1(get_group_size<FP>(m_M1)),
                  m_group_size_2(get_group_size<FP>(m_M2)),
                  m_FFT_1(m_M1, m_shared_size_1, m_M1 <= m_shared_size_1, m_group_size_1),
                  m_FFT_2(m_M2, m_shared_size_2, m_M2 <= m_shared_size_2, m_group_size_2)

        {
                if (m_N1 < 1 || m_N2 < 1)
                {
                        error("FFT size error: " + std::to_string(m_N1) + "x" + std::to_string(m_N2));
                }

                if (tex)
                {
                        m_TextureHandle = tex->get_texture().get_image_resident_handle_read_write_RGBA32F();
                }

                // Для обратного преобразования нужна корректировка данных с умножением на коэффициент,
                // так как разный размер у исходного вектора N и его расширенного M.
                double M1_Div_N1 = static_cast<double>(m_M1) / m_N1;
                double M2_Div_N2 = static_cast<double>(m_M2) / m_N2;

                // Compute the diagonal D in Lemma 13.2: use the radix-2 FFT
                // Формулы 13.13, 13.26.

                m_D1_fwd.load(conv<FP>(compute_h2(m_N1, m_M1, compute_h(m_N1, false, 1.0))));
                fft1d<1>(false, 1, &m_D1_fwd);

                m_D1_inv.load(conv<FP>(compute_h2(m_N1, m_M1, compute_h(m_N1, true, M1_Div_N1))));
                fft1d<1>(true, 1, &m_D1_inv);

                m_D2_fwd.load(conv<FP>(compute_h2(m_N2, m_M2, compute_h(m_N2, false, 1.0))));
                fft1d<2>(false, 1, &m_D2_fwd);

                m_D2_inv.load(conv<FP>(compute_h2(m_N2, m_M2, compute_h(m_N2, true, M2_Div_N2))));
                fft1d<2>(true, 1, &m_D2_inv);
        }
};
}

std::unique_ptr<IFourierGL1> create_fft_gl2d(int x, int y)
{
        return std::make_unique<GL2D<float>>(x, y, nullptr);
}

std::unique_ptr<IFourierGL2> create_fft_gl2d(int x, int y, const TextureRGBA32F& tex)
{
        return std::make_unique<GL2D<float>>(x, y, &tex);
}
