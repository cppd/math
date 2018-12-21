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

#include "program.h"

#include "com/bits.h"
#include "com/print.h"
#include "gpgpu/com/groups.h"

// clang-format off
constexpr const char dft_fft_global_shader[]
{
#include "dft_fft_global.comp.str"
};
constexpr const char dft_fft_shared_shader[]
{
#include "dft_fft_shared.comp.str"
};
constexpr const char dft_bit_reverse_shader[]
{
#include "dft_bit_reverse.comp.str"
};
constexpr const char dft_copy_input_shader[]
{
#include "dft_copy_input.comp.str"
};
constexpr const char dft_copy_output_shader[]
{
#include "dft_copy_output.comp.str"
};
constexpr const char dft_mul_shader[]
{
#include "dft_mul.comp.str"
};
constexpr const char dft_mul_d_shader[]
{
#include "dft_mul_d.comp.str"
};
// clang-format on

namespace
{
template <typename T>
std::string floating_point_source();

template <>
std::string floating_point_source<float>()
{
        std::string s;
        s += "#define complex vec2\n";
        s += "#define float_point float\n";
        s += "const float PI = " + std::string(PI_STR) + ";\n";
        s += "\n";
        return s;
}

template <>
std::string floating_point_source<double>()
{
        std::string s;
        s += "#define complex dvec2\n";
        s += "#define float_point double\n";
        s += "const double PI = " + std::string(PI_STR) + "LF;\n";
        s += "\n";
        return s;
}

std::string group_size_string(int group_size)
{
        return "const uint GROUP_SIZE = " + to_string(group_size) + ";\n";
}

std::string group_size_string(vec2i group_size)
{
        return "const uvec2 GROUP_SIZE = uvec2(" + to_string(group_size[0]) + ", " + to_string(group_size[1]) + ");\n";
}

std::string function_index_string(int index)
{
        return "const int FUNCTION_INDEX = " + to_string(index) + ";\n";
}

std::string n_m_string(int n1, int n2, int m1, int m2)
{
        std::string s;
        s += "const int N1 = " + to_string(n1) + ";\n";
        s += "const int N2 = " + to_string(n2) + ";\n";
        s += "const int M1 = " + to_string(m1) + ";\n";
        s += "const int M2 = " + to_string(m2) + ";\n";
        return s;
}

template <typename T>
std::string bit_reverse_source(int group_size)
{
        std::string s;
        s += floating_point_source<T>();
        s += group_size_string(group_size);
        return s + dft_bit_reverse_shader;
}

template <typename T>
std::string fft_global_source(int group_size)
{
        std::string s;
        s += floating_point_source<T>();
        s += group_size_string(group_size);
        return s + dft_fft_global_shader;
}

template <typename T>
std::string rows_mul_to_buffer_source(vec2i group_size, int n1, int n2, int m1, int m2)
{
        std::string s;
        s += floating_point_source<T>();
        s += group_size_string(group_size);
        s += function_index_string(0);
        s += n_m_string(n1, n2, m1, m2);
        return s + dft_mul_shader;
}

template <typename T>
std::string rows_mul_fr_buffer_source(vec2i group_size, int n1, int n2, int m1, int m2)
{
        std::string s;
        s += floating_point_source<T>();
        s += group_size_string(group_size);
        s += function_index_string(1);
        s += n_m_string(n1, n2, m1, m2);
        return s + dft_mul_shader;
}

template <typename T>
std::string cols_mul_to_buffer_source(vec2i group_size, int n1, int n2, int m1, int m2)
{
        std::string s;
        s += floating_point_source<T>();
        s += group_size_string(group_size);
        s += function_index_string(2);
        s += n_m_string(n1, n2, m1, m2);
        return s + dft_mul_shader;
}

template <typename T>
std::string cols_mul_fr_buffer_source(vec2i group_size, int n1, int n2, int m1, int m2)
{
        std::string s;
        s += floating_point_source<T>();
        s += group_size_string(group_size);
        s += function_index_string(3);
        s += n_m_string(n1, n2, m1, m2);
        return s + dft_mul_shader;
}

template <typename T>
std::string rows_mul_d_source(vec2i group_size)
{
        std::string s;
        s += floating_point_source<T>();
        s += group_size_string(group_size);
        return s + dft_mul_d_shader;
}

template <typename T>
std::string copy_input_source(vec2i group_size)
{
        std::string s;
        s += floating_point_source<T>();
        s += group_size_string(group_size);
        return s + dft_copy_input_shader;
}

template <typename T>
std::string copy_output_source(vec2i group_size)
{
        std::string s;
        s += floating_point_source<T>();
        s += group_size_string(group_size);
        return s + dft_copy_output_shader;
}

template <typename T>
std::string fft_shared_source(int n, int n_bits, int shared_size, int group_size, bool reverse_input)
{
        std::string s;
        s += floating_point_source<T>();
        s += "const uint N = " + to_string(n) + ";\n";
        s += "const uint N_MASK = " + to_string(n - 1) + ";\n";
        s += "const uint N_BITS = " + to_string(n_bits) + ";\n";
        s += "const uint SHARED_SIZE = " + to_string(shared_size) + ";\n";
        s += "const uint GROUP_SIZE = " + to_string(group_size) + ";\n";
        s += "const bool REVERSE_INPUT = " + (reverse_input ? std::string("true") : std::string("false")) + ";\n";
        return s + dft_fft_shared_shader;
}
}

//

template <typename T>
DeviceProgBitReverse<T>::DeviceProgBitReverse(int group_size)
        : m_group_size(group_size),
          m_bit_reverse(opengl::ComputeShader(bit_reverse_source<T>(group_size))),
          m_shader_memory(sizeof(ShaderMemory))
{
}

template <typename T>
void DeviceProgBitReverse<T>::exec(int max_threads, int n_mask, int n_bits, const DeviceMemory<std::complex<T>>& data) const
{
        ShaderMemory m;
        m.max_threads = max_threads;
        m.n_mask = n_mask;
        m.n_bits = n_bits;
        m_shader_memory.copy(m);

        m_shader_memory.bind(0);
        data.bind(1);

        m_bit_reverse.dispatch_compute(group_count(max_threads, m_group_size), 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

template <typename T>
DeviceProgFFTGlobal<T>::DeviceProgFFTGlobal(int group_size)
        : m_group_size(group_size),
          m_fft(opengl::ComputeShader(fft_global_source<T>(group_size))),
          m_shader_memory(sizeof(ShaderMemory))
{
}

template <typename T>
void DeviceProgFFTGlobal<T>::exec(int max_threads, bool inverse, T two_pi_div_m, int n_div_2_mask, int m_div_2,
                                  const DeviceMemory<std::complex<T>>& data) const
{
        ShaderMemory m;
        m.inverse_dft = inverse;
        m.max_threads = max_threads;
        m.n_div_2_mask = n_div_2_mask;
        m.m_div_2 = m_div_2;
        m.two_pi_div_m = two_pi_div_m;
        m_shader_memory.copy(m);

        m_shader_memory.bind(0);
        data.bind(1);

        m_fft.dispatch_compute(group_count(max_threads, m_group_size), 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

template <typename T>
DeviceProgCopyInput<T>::DeviceProgCopyInput(vec2i group_size, int n1, int n2)
        : m_group_count(group_count(n1, n2, group_size)),
          m_copy_input(opengl::ComputeShader(copy_input_source<T>(group_size))),
          m_shader_memory(sizeof(ShaderMemory))
{
}

template <typename T>
void DeviceProgCopyInput<T>::copy(bool source_srgb, const GLuint64 tex, const DeviceMemory<std::complex<T>>& data)
{
        ShaderMemory m;
        m.source_srgb = source_srgb;
        m_shader_memory.copy(m);

        m_copy_input.set_uniform_handle(0, tex);
        m_shader_memory.bind(0);
        data.bind(1);

        m_copy_input.dispatch_compute(m_group_count[0], m_group_count[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

template <typename T>
DeviceProgCopyOutput<T>::DeviceProgCopyOutput(vec2i group_size, int n1, int n2)
        : m_group_count(group_count(n1, n2, group_size)),
          m_copy_output(opengl::ComputeShader(copy_output_source<T>(group_size))),
          m_shader_memory(sizeof(ShaderMemory))
{
}

template <typename T>
void DeviceProgCopyOutput<T>::copy(T to_mul, const GLuint64 tex, const DeviceMemory<std::complex<T>>& data)
{
        ShaderMemory m;
        m.to_mul = to_mul;
        m_shader_memory.copy(m);

        m_copy_output.set_uniform_handle(0, tex);
        m_shader_memory.bind(0);
        data.bind(1);

        m_copy_output.dispatch_compute(m_group_count[0], m_group_count[1], 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

//

template <typename T>
DeviceProgMul<T>::DeviceProgMul(vec2i group_size, int n1, int n2, int m1, int m2)
        : m_rows_to_buffer_groups(group_count(m1, n2, group_size)),
          m_rows_from_buffer_groups(group_count(n1, n2, group_size)),
          m_columns_to_buffer_groups(group_count(n1, m2, group_size)),
          m_columns_from_buffer_groups(group_count(n1, n2, group_size)),
          m_rows_to_buffer(opengl::ComputeShader(rows_mul_to_buffer_source<T>(group_size, n1, n2, m1, m2))),
          m_rows_from_buffer(opengl::ComputeShader(rows_mul_fr_buffer_source<T>(group_size, n1, n2, m1, m2))),
          m_columns_to_buffer(opengl::ComputeShader(cols_mul_to_buffer_source<T>(group_size, n1, n2, m1, m2))),
          m_columns_from_buffer(opengl::ComputeShader(cols_mul_fr_buffer_source<T>(group_size, n1, n2, m1, m2))),
          m_shader_memory(sizeof(ShaderMemory))
{
}

template <typename T>
void DeviceProgMul<T>::set_and_bind(bool inverse, const DeviceMemory<std::complex<T>>& data,
                                    const DeviceMemory<std::complex<T>>& buffer) const
{
        ShaderMemory m;
        m.inverse_dft = inverse;
        m_shader_memory.copy(m);

        m_shader_memory.bind(0);
        data.bind(1);
        buffer.bind(2);
}

template <typename T>
void DeviceProgMul<T>::rows_to_buffer(bool inverse, const DeviceMemory<std::complex<T>>& data,
                                      const DeviceMemory<std::complex<T>>& buffer) const
{
        set_and_bind(inverse, data, buffer);

        m_rows_to_buffer.dispatch_compute(m_rows_to_buffer_groups[0], m_rows_to_buffer_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

template <typename T>
void DeviceProgMul<T>::rows_from_buffer(bool inverse, const DeviceMemory<std::complex<T>>& data,
                                        const DeviceMemory<std::complex<T>>& buffer) const
{
        set_and_bind(inverse, data, buffer);

        m_rows_from_buffer.dispatch_compute(m_rows_from_buffer_groups[0], m_rows_from_buffer_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

template <typename T>
void DeviceProgMul<T>::columns_to_buffer(bool inverse, const DeviceMemory<std::complex<T>>& data,
                                         const DeviceMemory<std::complex<T>>& buffer) const
{
        set_and_bind(inverse, data, buffer);

        m_columns_to_buffer.dispatch_compute(m_columns_to_buffer_groups[0], m_columns_to_buffer_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

template <typename T>
void DeviceProgMul<T>::columns_from_buffer(bool inverse, const DeviceMemory<std::complex<T>>& data,
                                           const DeviceMemory<std::complex<T>>& buffer) const
{
        set_and_bind(inverse, data, buffer);

        m_columns_from_buffer.dispatch_compute(m_columns_from_buffer_groups[0], m_columns_from_buffer_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

template <typename T>
DeviceProgMulD<T>::DeviceProgMulD(vec2i group_size, int n1, int n2, int m1, int m2)
        : m_row_groups(group_count(m1, n2, group_size)),
          m_column_groups(group_count(m2, n1, group_size)),
          m_mul_d(opengl::ComputeShader(rows_mul_d_source<T>(group_size))),
          m_memory_rows(sizeof(ShaderMemory)),
          m_memory_columns(sizeof(ShaderMemory))
{
        {
                ShaderMemory m;
                m.columns = m1;
                m.rows = n2;
                m_memory_rows.copy(m);
        }
        {
                ShaderMemory m;
                m.columns = m2;
                m.rows = n1;
                m_memory_columns.copy(m);
        }
}

template <typename T>
void DeviceProgMulD<T>::rows_mul_d(const DeviceMemory<std::complex<T>>& d, const DeviceMemory<std::complex<T>>& data) const
{
        m_memory_rows.bind(0);
        d.bind(1);
        data.bind(2);

        m_mul_d.dispatch_compute(m_row_groups[0], m_row_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

template <typename T>
void DeviceProgMulD<T>::columns_mul_d(const DeviceMemory<std::complex<T>>& d, const DeviceMemory<std::complex<T>>& data) const
{
        m_memory_columns.bind(0);
        d.bind(1);
        data.bind(2);

        m_mul_d.dispatch_compute(m_column_groups[0], m_column_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

template <typename T>
DeviceProgFFTShared<T>::DeviceProgFFTShared(int n, int shared_size, int group_size, bool reverse_input)
        : m_n(n),
          m_n_bits(binary_size(n)),
          m_shared_size(shared_size),
          m_fft(opengl::ComputeShader(fft_shared_source<T>(m_n, m_n_bits, m_shared_size, group_size, reverse_input))),
          m_shader_memory(sizeof(ShaderMemory))
{
        ASSERT((1 << m_n_bits) == m_n);
}

template <typename T>
void DeviceProgFFTShared<T>::exec(bool inverse, int data_size, const DeviceMemory<std::complex<T>>& data) const
{
        ShaderMemory m;
        m.inverse_dft = inverse;
        m.data_size = data_size;
        m_shader_memory.copy(m);

        m_shader_memory.bind(0);
        data.bind(1);

        m_fft.dispatch_compute(group_count(data_size, m_shared_size), 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

template class DeviceProgBitReverse<float>;
template class DeviceProgBitReverse<double>;
template class DeviceProgFFTGlobal<float>;
template class DeviceProgFFTGlobal<double>;
template class DeviceProgCopyInput<float>;
template class DeviceProgCopyInput<double>;
template class DeviceProgCopyOutput<float>;
template class DeviceProgCopyOutput<double>;
template class DeviceProgMul<float>;
template class DeviceProgMul<double>;
template class DeviceProgMulD<float>;
template class DeviceProgMulD<double>;
template class DeviceProgFFTShared<float>;
template class DeviceProgFFTShared<double>;
