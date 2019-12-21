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

#if defined(OPENGL_FOUND)

#include "compute.h"

#include "compute_memory.h"
#include "compute_program.h"

#include "com/error.h"
#include "com/groups.h"
#include "com/log.h"
#include "com/print.h"
#include "com/time.h"
#include "gpu/dft/com/com.h"
#include "graphics/opengl/buffers.h"
#include "graphics/opengl/query.h"
#include "graphics/opengl/time.h"

#include <functional>
#include <optional>

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

int shared_size(int dft_size)
{
        return dft_shared_size<std::complex<float>>(dft_size, opengl::max_compute_shared_memory());
}

int group_size(int dft_size)
{
        return dft_group_size<std::complex<float>>(dft_size, opengl::max_fixed_group_size_x(),
                                                   opengl::max_fixed_group_invocations(), opengl::max_compute_shared_memory());
}

class Fft1d
{
        int m_n;
        int m_n_shared;
        bool m_only_shared;
        DftProgramFftShared m_fft;
        std::optional<DftProgramBitReverse> m_bit_reverse;
        std::optional<DftProgramFftGlobal> m_fft_g;
        std::optional<DftMemoryFftGlobalBuffer> m_fft_g_memory_buffer;
        std::vector<DftMemoryFftGlobalData> m_fft_g_memory_data;

public:
        Fft1d(int count, int n)
                : m_n(n),
                  m_n_shared(shared_size(n)),
                  m_only_shared(n <= m_n_shared),
                  m_fft(count, n, shared_size(n), group_size(n), m_only_shared)
        {
                if (m_only_shared)
                {
                        return;
                }

                m_bit_reverse.emplace(GROUP_SIZE_1D, count, n);
                m_fft_g.emplace(count, n, GROUP_SIZE_1D);
                m_fft_g_memory_buffer.emplace();

                // Половина размера текущих отдельных ДПФ
                int m_div_2 = m_n_shared;
                float two_pi_div_m = PI<float> / m_div_2;
                for (; m_div_2 < m_n; m_div_2 <<= 1, two_pi_div_m /= 2)
                {
                        m_fft_g_memory_data.emplace_back(two_pi_div_m, m_div_2);
                }
                ASSERT(m_fft_g_memory_data.size() > 0);
                ASSERT(m_n == (m_n_shared << m_fft_g_memory_data.size()));
        }

        void exec(bool inverse, const DeviceMemory<std::complex<float>>* data)
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

                m_fft_g_memory_buffer->set(*data);
                m_fft_g_memory_buffer->bind();
                // Досчитать до нужного размера уже в глобальной памяти без разделяемой
                for (const DftMemoryFftGlobalData& memory_data : m_fft_g_memory_data)
                {
                        memory_data.bind();
                        m_fft_g->exec(inverse);
                }
        }
};

class Impl final : public DFTCompute, public DFTComputeTexture
{
        const int m_n1, m_n2, m_m1, m_m2;
        DeviceMemory<std::complex<float>> m_d1_fwd, m_d1_inv, m_d2_fwd, m_d2_inv;
        DeviceMemory<std::complex<float>> m_x_d, m_buffer;
        std::optional<DftProgramCopyInput> m_copy_input;
        std::optional<DftProgramCopyOutput> m_copy_output;
        DftProgramMul m_mul;
        DftProgramMulD m_mul_d;
        Fft1d m_fft_n2_m1;
        Fft1d m_fft_n1_m2;

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

                m_x_d.write(*src);

                glFinish();

                double start_time = time_in_seconds();

                dft2d<true>(inverse);

                glFinish();

                LOG("calc OpenGL: " + to_string_fixed(1000.0 * (time_in_seconds() - start_time), 5) + " ms");

                m_x_d.read(src);
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

                Fft1d fft_1_m1(1, m_m1);
                Fft1d fft_1_m2(1, m_m2);

                // Compute the diagonal D in Lemma 13.2: use the radix-2 FFT
                // Формулы 13.13, 13.26.

                m_d1_fwd.write(conv<float>(dft_compute_h2(m_n1, m_m1, dft_compute_h(m_n1, false, 1.0))));
                fft_1_m1.exec(false, &m_d1_fwd);

                m_d1_inv.write(conv<float>(dft_compute_h2(m_n1, m_m1, dft_compute_h(m_n1, true, m1_div_n1))));
                fft_1_m1.exec(true, &m_d1_inv);

                m_d2_fwd.write(conv<float>(dft_compute_h2(m_n2, m_m2, dft_compute_h(m_n2, false, 1.0))));
                fft_1_m2.exec(false, &m_d2_fwd);

                m_d2_inv.write(conv<float>(dft_compute_h2(m_n2, m_m2, dft_compute_h(m_n2, true, m2_div_n2))));
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
                  m_m1(dft_compute_m(m_n1)),
                  m_m2(dft_compute_m(m_n2)),
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
                        m_copy_output.emplace(GROUP_SIZE_2D, *result, m_n1, m_n2, static_cast<float>(1.0 / (1ull * m_n1 * m_n2)));

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
        return std::make_unique<Impl>(0, 0, width, height, nullptr, nullptr);
}

std::unique_ptr<DFTComputeTexture> create_dft_compute_texture(const opengl::Texture& source, unsigned x, unsigned y,
                                                              unsigned width, unsigned height, const opengl::Texture& result)
{
        return std::make_unique<Impl>(x, y, width, height, &source, &result);
}
}

#endif
