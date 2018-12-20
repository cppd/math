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
        : m_group_size(group_size), m_bit_reverse(opengl::ComputeShader(bit_reverse_source<T>(group_size)))
{
}

template <typename T>
void DeviceProgBitReverse<T>::exec(int max_threads, int n_mask, int n_bits, DeviceMemory<std::complex<T>>* data) const
{
        m_bit_reverse.set_uniform_unsigned(0, max_threads);
        m_bit_reverse.set_uniform_unsigned(1, n_mask);
        m_bit_reverse.set_uniform_unsigned(2, n_bits);
        data->bind(0);
        m_bit_reverse.dispatch_compute(group_count(max_threads, m_group_size), 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

template <typename T>
DeviceProgFFTGlobal<T>::DeviceProgFFTGlobal(int group_size)
        : m_group_size(group_size), m_fft(opengl::ComputeShader(fft_global_source<T>(group_size)))
{
}

template <typename T>
void DeviceProgFFTGlobal<T>::exec(int max_threads, bool inverse, T two_pi_div_m, int n_div_2_mask, int m_div_2,
                                  DeviceMemory<std::complex<T>>* data) const
{
        m_fft.set_uniform(0, inverse);
        m_fft.set_uniform_unsigned(1, max_threads);
        m_fft.set_uniform_unsigned(2, n_div_2_mask);
        m_fft.set_uniform_unsigned(3, m_div_2);
        m_fft.set_uniform(4, two_pi_div_m);
        data->bind(0);
        m_fft.dispatch_compute(group_count(max_threads, m_group_size), 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

template <typename T>
DeviceProgCopy<T>::DeviceProgCopy(vec2i group_size, int n1, int n2)
        : m_group_count(group_count(n1, n2, group_size)),
          m_copy_input(opengl::ComputeShader(copy_input_source<T>(group_size))),
          m_copy_output(opengl::ComputeShader(copy_output_source<T>(group_size)))
{
}

template <typename T>
void DeviceProgCopy<T>::copy_input(bool source_srgb, const GLuint64 tex, DeviceMemory<std::complex<T>>* data)
{
        m_copy_input.set_uniform(0, source_srgb);
        m_copy_input.set_uniform_handle(1, tex);
        data->bind(0);
        m_copy_input.dispatch_compute(m_group_count[0], m_group_count[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

template <typename T>
void DeviceProgCopy<T>::copy_output(T to_mul, const GLuint64 tex, const DeviceMemory<std::complex<T>>& data)
{
        m_copy_output.set_uniform(0, to_mul);
        m_copy_output.set_uniform_handle(1, tex);
        data.bind(0);
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
          m_columns_from_buffer(opengl::ComputeShader(cols_mul_fr_buffer_source<T>(group_size, n1, n2, m1, m2)))
{
}

template <typename T>
void DeviceProgMul<T>::rows_to_buffer(bool inverse, const DeviceMemory<std::complex<T>>& data,
                                      DeviceMemory<std::complex<T>>* buffer) const
{
        m_rows_to_buffer.set_uniform(0, inverse);
        data.bind(0);
        buffer->bind(1);
        m_rows_to_buffer.dispatch_compute(m_rows_to_buffer_groups[0], m_rows_to_buffer_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

template <typename T>
void DeviceProgMul<T>::rows_from_buffer(bool inverse, DeviceMemory<std::complex<T>>* data,
                                        const DeviceMemory<std::complex<T>>& buffer) const
{
        m_rows_from_buffer.set_uniform(0, inverse);
        data->bind(0);
        buffer.bind(1);
        m_rows_from_buffer.dispatch_compute(m_rows_from_buffer_groups[0], m_rows_from_buffer_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

template <typename T>
void DeviceProgMul<T>::columns_to_buffer(bool inverse, const DeviceMemory<std::complex<T>>& data,
                                         DeviceMemory<std::complex<T>>* buffer) const
{
        m_columns_to_buffer.set_uniform(0, inverse);
        data.bind(0);
        buffer->bind(1);
        m_columns_to_buffer.dispatch_compute(m_columns_to_buffer_groups[0], m_columns_to_buffer_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

template <typename T>
void DeviceProgMul<T>::columns_from_buffer(bool inverse, DeviceMemory<std::complex<T>>* data,
                                           const DeviceMemory<std::complex<T>>& buffer) const
{
        m_columns_from_buffer.set_uniform(0, inverse);
        data->bind(0);
        buffer.bind(1);
        m_columns_from_buffer.dispatch_compute(m_columns_from_buffer_groups[0], m_columns_from_buffer_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

template <typename T>
DeviceProgMulD<T>::DeviceProgMulD(vec2i group_size, int n1, int n2, int m1, int m2)
        : m_n1(n1),
          m_n2(n2),
          m_m1(m1),
          m_m2(m2),
          m_row_groups(group_count(m1, n2, group_size)),
          m_column_groups(group_count(m2, n1, group_size)),
          m_mul_d(opengl::ComputeShader(rows_mul_d_source<T>(group_size)))
{
}

template <typename T>
void DeviceProgMulD<T>::rows_mul_d(const DeviceMemory<std::complex<T>>& d, DeviceMemory<std::complex<T>>* data) const
{
        m_mul_d.set_uniform(0, m_m1);
        m_mul_d.set_uniform(1, m_n2);
        d.bind(0);
        data->bind(1);
        m_mul_d.dispatch_compute(m_row_groups[0], m_row_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

template <typename T>
void DeviceProgMulD<T>::columns_mul_d(const DeviceMemory<std::complex<T>>& d, DeviceMemory<std::complex<T>>* data) const
{
        m_mul_d.set_uniform(0, m_m2);
        m_mul_d.set_uniform(1, m_n1);
        d.bind(0);
        data->bind(1);
        m_mul_d.dispatch_compute(m_column_groups[0], m_column_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

template <typename T>
DeviceProgFFTShared<T>::DeviceProgFFTShared(int n, int shared_size, int group_size, bool reverse_input)
        : m_n(n),
          m_n_bits(binary_size(n)),
          m_shared_size(shared_size),
          m_fft(opengl::ComputeShader(fft_shared_source<T>(m_n, m_n_bits, m_shared_size, group_size, reverse_input)))
{
        ASSERT((1 << m_n_bits) == m_n);
}

template <typename T>
void DeviceProgFFTShared<T>::exec(bool inverse, int data_size, DeviceMemory<std::complex<T>>* global_data) const
{
        m_fft.set_uniform(0, inverse);
        m_fft.set_uniform_unsigned(1, data_size);
        global_data->bind(0);
        m_fft.dispatch_compute(group_count(data_size, m_shared_size), 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

template class DeviceProgBitReverse<float>;
template class DeviceProgBitReverse<double>;
template class DeviceProgFFTGlobal<float>;
template class DeviceProgFFTGlobal<double>;
template class DeviceProgCopy<float>;
template class DeviceProgCopy<double>;
template class DeviceProgMul<float>;
template class DeviceProgMul<double>;
template class DeviceProgMulD<float>;
template class DeviceProgMulD<double>;
template class DeviceProgFFTShared<float>;
template class DeviceProgFFTShared<double>;
