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

#include "compute.h"

#include "compute_program.h"

#include "com/bits.h"
#include "com/error.h"
#include "com/groups.h"
#include "com/log.h"
#include "com/math.h"
#include "com/print.h"
#include "com/time.h"
#include "graphics/opengl/buffers.h"
#include "graphics/opengl/query.h"
#include "graphics/opengl/time.h"

#include <complex>
#include <optional>
#include <sstream>
#include <type_traits>
#include <vector>

constexpr const int GROUP_SIZE_1D = 256;
constexpr const vec2i GROUP_SIZE_2D = vec2i(16, 16);

namespace gpu_opengl
{
namespace
{
class Command
{
        std::string m_description;
        std::function<void()> m_command;

public:
        Command(std::string&& description, std::function<void()>&& command)
                : m_description(std::move(description)), m_command(std::move(command))
        {
        }
        void run() const
        {
                m_command();
        }
        const std::string& description() const
        {
                return m_description;
        }
};

class Commands
{
        std::vector<Command> m_commands;
        std::string m_before_text;
        std::string m_sum_text;

public:
        void clear()
        {
                m_commands.clear();
        }

        void add(const char* description, std::function<void()>&& command)
        {
                m_commands.emplace_back(description, std::move(command));
        }

        void set_before_text(const std::string& s)
        {
                m_before_text = s;
        }

        void set_sum_text(const std::string& s)
        {
                m_sum_text = s;
        }

        void run_with_time(opengl::TimeElapsed& time_elapsed) const
        {
                double time = 0;
                for (const Command& command : m_commands)
                {
                        {
                                opengl::TimeElapsedRun r(time_elapsed);
                                command.run();
                        }
                        double elapsed = time_elapsed.milliseconds();
                        time += elapsed;
                        LOG(m_before_text + command.description() + to_string_fixed(elapsed, 5) + " ms");
                }
                LOG(m_sum_text + to_string_fixed(time, 5) + " ms");
        }

        void run() const
        {
                for (const Command& command : m_commands)
                {
                        command.run();
                }
        }
};

template <typename T>
class DeviceMemory final
{
        size_t m_size;
        opengl::Buffer m_buffer;

public:
        DeviceMemory(int size) : m_size(size), m_buffer(size * sizeof(T), GL_MAP_WRITE_BIT | GL_MAP_READ_BIT)
        {
        }

        void write(const std::vector<T>& data) const
        {
                if (data.size() != m_size)
                {
                        error("Storage size error");
                }
                opengl::map_and_write_to_buffer(m_buffer, data);
        }

        void read(std::vector<T>* data) const
        {
                if (data->size() != m_size)
                {
                        error("Storage size error");
                }
                opengl::map_and_read_from_buffer(m_buffer, data);
        }

        operator const opengl::Buffer&() const
        {
                return m_buffer;
        }
};

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
class Fft1d
{
        int m_n;
        int m_n_shared;
        bool m_only_shared;
        DftProgramFftShared<FP> m_fft;
        std::optional<DftProgramBitReverse<FP>> m_bit_reverse;
        std::optional<DftProgramFftGlobal<FP>> m_fft_g;
        std::optional<DftMemoryFftGlobal<FP>> m_fft_g_memory;

public:
        Fft1d(int count, int n)
                : m_n(n),
                  m_n_shared(shared_size<FP>(n)),
                  m_only_shared(n <= m_n_shared),
                  m_fft(count, n, shared_size<FP>(n), group_size<FP>(n), m_only_shared)
        {
                ASSERT((1 << m_fft.n_bits()) == m_fft.n());
                ASSERT(m_n_shared == m_fft.shared_size());
                ASSERT(m_only_shared == m_fft.reverse_input());

                m_bit_reverse.emplace(GROUP_SIZE_1D, count, n);
                m_fft_g.emplace(count, n, GROUP_SIZE_1D);
                m_fft_g_memory.emplace();
                ASSERT(m_bit_reverse->n() == n);
                ASSERT(m_bit_reverse->n() == m_fft_g->n());
                ASSERT(m_bit_reverse->count() == m_fft_g->count());
                ASSERT(m_fft.n() == m_fft_g->n());
                ASSERT(m_fft.count() == m_fft_g->count());
                if (m_only_shared)
                {
                        m_fft_g_memory.reset();
                        m_fft_g.reset();
                        m_bit_reverse.reset();
                }
        }

        void exec(bool inverse, const DeviceMemory<std::complex<FP>>* data)
        {
                if (m_n == 1)
                {
                        return;
                }

                if (m_only_shared)
                {
                        m_fft.exec(inverse, *data);
                        return;
                }

                // Если n превышает максимум обрабатываемых данных shared_size, то вначале
                // надо отдельно выполнить перестановку данных, а потом запускать функции
                // с отключенной перестановкой, иначе одни запуски будут вносить изменения
                // в данные других запусков, так как результат пишется в исходные данные.
                m_bit_reverse->exec(*data);
                m_fft.exec(inverse, *data);

                // Досчитать до нужного размера уже в глобальной памяти без разделяемой
                m_fft_g_memory->set_buffer(*data);
                int m_div_2 = m_n_shared; // половина размера текущих отдельных БПФ
                FP two_pi_div_m = PI<FP> / m_div_2;
                for (; m_div_2 < m_n; m_div_2 <<= 1, two_pi_div_m /= 2)
                {
                        m_fft_g_memory->set_data(two_pi_div_m, m_div_2);
                        m_fft_g->exec(inverse, *m_fft_g_memory);
                }
        }
};

template <typename FP>
class Impl final : public DFTCompute, public DFTComputeTexture
{
        const int m_n1, m_n2, m_m1, m_m2, m_m1_bin, m_m2_bin;
        DeviceMemory<std::complex<FP>> m_d1_fwd, m_d1_inv, m_d2_fwd, m_d2_inv;
        DeviceMemory<std::complex<FP>> m_x_d, m_buffer;
        std::optional<DftProgramCopyInput<FP>> m_copy_input;
        std::optional<DftProgramCopyOutput<FP>> m_copy_output;
        DftProgramMul<FP> m_mul;
        DftProgramMulD<FP> m_mul_d;
        Fft1d<FP> m_fft_n2_m1;
        Fft1d<FP> m_fft_n1_m2;

        std::optional<opengl::TimeElapsed> m_time_elapsed;

        Commands m_commands_forward;
        Commands m_commands_inverse;

        template <bool WithTime>
        void dft2d(bool inverse)
        {
                const Commands* commands = inverse ? &m_commands_inverse : &m_commands_forward;

                if constexpr (WithTime)
                {
                        commands->run_with_time(*m_time_elapsed);
                }
                else
                {
                        commands->run();
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

                dft2d<true>(inverse);

                glFinish();

                LOG("calc OpenGL: " + to_string_fixed(1000.0 * (time_in_seconds() - start_time), 5) + " ms");

                m_x_d.read(&data);

                *src = conv<float>(std::move(data));
        }

        void exec() override
        {
                m_copy_input->copy(m_x_d);
                dft2d<false>(false /*inverse*/);
                m_copy_output->copy(m_x_d);
        }

        void compute_diagonals()
        {
                // Для обратного преобразования нужна корректировка данных с умножением на коэффициент,
                // так как разный размер у исходного вектора N и его расширенного M.
                double m1_div_n1 = static_cast<double>(m_m1) / m_n1;
                double m2_div_n2 = static_cast<double>(m_m2) / m_n2;

                Fft1d<FP> fft_1_m1(1, m_m1);
                Fft1d<FP> fft_1_m2(1, m_m2);

                // Compute the diagonal D in Lemma 13.2: use the radix-2 FFT
                // Формулы 13.13, 13.26.

                m_d1_fwd.write(conv<FP>(compute_h2(m_n1, m_m1, compute_h(m_n1, false, 1.0))));
                fft_1_m1.exec(false, &m_d1_fwd);

                m_d1_inv.write(conv<FP>(compute_h2(m_n1, m_m1, compute_h(m_n1, true, m1_div_n1))));
                fft_1_m1.exec(true, &m_d1_inv);

                m_d2_fwd.write(conv<FP>(compute_h2(m_n2, m_m2, compute_h(m_n2, false, 1.0))));
                fft_1_m2.exec(false, &m_d2_fwd);

                m_d2_inv.write(conv<FP>(compute_h2(m_n2, m_m2, compute_h(m_n2, true, m2_div_n2))));
                fft_1_m2.exec(true, &m_d2_inv);
        }

        void record_commands(bool inverse, Commands* commands)
        {
                commands->clear();

                commands->set_before_text(" ");
                commands->set_sum_text(" all       : ");

                if (m_n1 > 1)
                {
                        // clang-format off
                        commands->add("row mul to: ", [=, this]()
                        {
                                m_mul.rows_to_buffer(inverse, m_x_d, m_buffer);
                        });
                        commands->add("row fft1d : ", [=, this]()
                        {
                                m_fft_n2_m1.exec(inverse, &m_buffer);
                        });
                        commands->add("row mul d : ", [=, this]()
                        {
                                m_mul_d.rows_mul_d(inverse ? m_d1_inv : m_d1_fwd, m_buffer);
                        });
                        commands->add("row fft1d : ", [=, this]()
                        {
                                m_fft_n2_m1.exec(!inverse, &m_buffer);
                        });
                        commands->add("row mul fr: ", [=, this]()
                        {
                                m_mul.rows_from_buffer(inverse, m_x_d, m_buffer);
                        });
                        // clang-format on
                }
                if (m_n2 > 1)
                {
                        // clang-format off
                        commands->add("col mul to: ", [=, this]()
                        {
                                m_mul.columns_to_buffer(inverse, m_x_d, m_buffer);
                        });
                        commands->add("col fft1d : ", [=, this]()
                        {
                                m_fft_n1_m2.exec(inverse, &m_buffer);
                        });
                        commands->add("col mul d : ", [=, this]()
                        {
                                m_mul_d.columns_mul_d(inverse ? m_d2_inv : m_d2_fwd, m_buffer);
                        });
                        commands->add("col fft1d : ", [=, this]()
                        {
                                m_fft_n1_m2.exec(!inverse, &m_buffer);
                        });
                        commands->add("col mul fr: ", [=, this]()
                        {
                                m_mul.columns_from_buffer(inverse, m_x_d, m_buffer);
                        });
                        // clang-format on
                }
        }

        void record_commands()
        {
                record_commands(false, &m_commands_forward);
                record_commands(true, &m_commands_inverse);
        }

public:
        Impl(unsigned x, unsigned y, unsigned n1, unsigned n2, const opengl::Texture* source, const opengl::Texture* result)
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
                  m_mul(GROUP_SIZE_2D, m_n1, m_n2, m_m1, m_m2),
                  m_mul_d(GROUP_SIZE_2D, m_n1, m_n2, m_m1, m_m2),
                  m_fft_n2_m1(m_n2, m_m1),
                  m_fft_n1_m2(m_n1, m_m2)
        {
                if (m_n1 < 1 || m_n2 < 1)
                {
                        error("FFT size error: " + to_string(m_n1) + "x" + to_string(m_n2));
                }

                ASSERT(static_cast<bool>(source) == static_cast<bool>(result));
                if (source && result)
                {
                        m_copy_input.emplace(GROUP_SIZE_2D, *source, x, y, m_n1, m_n2);
                        m_copy_output.emplace(GROUP_SIZE_2D, *result, m_n1, m_n2, static_cast<FP>(1.0 / (1ull * m_n1 * m_n2)));

                        ASSERT(result->format() == GL_R32F);
                        ASSERT(result->width() == m_n1 && result->height() == m_n2);
                }
                else
                {
                        m_time_elapsed.emplace();
                }

                compute_diagonals();

                record_commands();
        }

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;
};
}
std::unique_ptr<DFTCompute> create_dft_compute(unsigned width, unsigned height)
{
        return std::make_unique<Impl<float>>(0, 0, width, height, nullptr, nullptr);
}

std::unique_ptr<DFTComputeTexture> create_dft_compute_texture(const opengl::Texture& source, unsigned x, unsigned y,
                                                              unsigned width, unsigned height, const opengl::Texture& result)
{
        return std::make_unique<Impl<float>>(x, y, width, height, &source, &result);
}
}
