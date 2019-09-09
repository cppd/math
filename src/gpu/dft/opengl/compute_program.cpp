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

#include "compute_program.h"

#include "shader_source.h"

#include "com/bits.h"
#include "com/groups.h"
#include "com/print.h"

namespace gpu_opengl
{
namespace
{
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
        s += group_size_string(group_size);
        return dft_bit_reverse_comp(s);
}

template <typename T>
std::string fft_global_source(int group_size)
{
        std::string s;
        s += group_size_string(group_size);
        return dft_fft_global_comp(s);
}

template <typename T>
std::string rows_mul_to_buffer_source(vec2i group_size, int n1, int n2, int m1, int m2)
{
        std::string s;
        s += group_size_string(group_size);
        s += function_index_string(0);
        s += n_m_string(n1, n2, m1, m2);
        return dft_mul_comp(s);
}

template <typename T>
std::string rows_mul_fr_buffer_source(vec2i group_size, int n1, int n2, int m1, int m2)
{
        std::string s;
        s += group_size_string(group_size);
        s += function_index_string(1);
        s += n_m_string(n1, n2, m1, m2);
        return dft_mul_comp(s);
}

template <typename T>
std::string cols_mul_to_buffer_source(vec2i group_size, int n1, int n2, int m1, int m2)
{
        std::string s;
        s += group_size_string(group_size);
        s += function_index_string(2);
        s += n_m_string(n1, n2, m1, m2);
        return dft_mul_comp(s);
}

template <typename T>
std::string cols_mul_fr_buffer_source(vec2i group_size, int n1, int n2, int m1, int m2)
{
        std::string s;
        s += group_size_string(group_size);
        s += function_index_string(3);
        s += n_m_string(n1, n2, m1, m2);
        return dft_mul_comp(s);
}

template <typename T>
std::string rows_mul_d_source(vec2i group_size)
{
        std::string s;
        s += group_size_string(group_size);
        return dft_mul_d_comp(s);
}

template <typename T>
std::string copy_input_source(vec2i group_size)
{
        std::string s;
        s += group_size_string(group_size);
        return dft_copy_input_comp(s);
}

template <typename T>
std::string copy_output_source(vec2i group_size)
{
        std::string s;
        s += group_size_string(group_size);
        return dft_copy_output_comp(s);
}

template <typename T>
std::string fft_shared_source(int n, int n_bits, int shared_size, int group_size, bool reverse_input)
{
        std::string s;
        s += "const uint N = " + to_string(n) + ";\n";
        s += "const uint N_MASK = " + to_string(n - 1) + ";\n";
        s += "const uint N_BITS = " + to_string(n_bits) + ";\n";
        s += "const uint SHARED_SIZE = " + to_string(shared_size) + ";\n";
        s += "const uint GROUP_SIZE = " + to_string(group_size) + ";\n";
        s += "const bool REVERSE_INPUT = " + (reverse_input ? std::string("true") : std::string("false")) + ";\n";
        return dft_fft_shared_comp(s);
}
}

//

template <typename T>
DftProgramBitReverse<T>::DftProgramBitReverse(int group_size)
        : m_group_size(group_size),
          m_bit_reverse(opengl::ComputeShader(bit_reverse_source<T>(group_size))),
          m_shader_memory(sizeof(ShaderMemory))
{
}

template <typename T>
void DftProgramBitReverse<T>::exec(int max_threads, int n_mask, int n_bits, const opengl::StorageBuffer& data) const
{
        ShaderMemory m;
        m.max_threads = max_threads;
        m.n_mask = n_mask;
        m.n_bits = n_bits;
        m_shader_memory.copy(m);

        m_shader_memory.bind(DATA_BINDING);
        data.bind(BUFFER_BINDING);

        m_bit_reverse.dispatch_compute(group_count(max_threads, m_group_size), 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

template <typename T>
DftProgramFftGlobal<T>::DftProgramFftGlobal(int group_size)
        : m_group_size(group_size),
          m_fft(opengl::ComputeShader(fft_global_source<T>(group_size))),
          m_shader_memory(sizeof(ShaderMemory))
{
}

template <typename T>
void DftProgramFftGlobal<T>::exec(int max_threads, bool inverse, T two_pi_div_m, int n_div_2_mask, int m_div_2,
                                  const opengl::StorageBuffer& data) const
{
        ShaderMemory m;
        m.inverse_dft = inverse;
        m.max_threads = max_threads;
        m.n_div_2_mask = n_div_2_mask;
        m.m_div_2 = m_div_2;
        m.two_pi_div_m = two_pi_div_m;
        m_shader_memory.copy(m);

        m_shader_memory.bind(DATA_BINDING);
        data.bind(BUFFER_BINDING);

        m_fft.dispatch_compute(group_count(max_threads, m_group_size), 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

template <typename T>
DftProgramCopyInput<T>::DftProgramCopyInput(vec2i group_size, int n1, int n2)
        : m_group_count(group_count(n1, n2, group_size)), m_copy_input(opengl::ComputeShader(copy_input_source<T>(group_size)))
{
}

template <typename T>
void DftProgramCopyInput<T>::copy(const GLuint64 tex, const opengl::StorageBuffer& data)
{
        m_copy_input.set_uniform_handle(SRC_IMAGE_LOCATION, tex);
        data.bind(BUFFER_BINDING);

        m_copy_input.dispatch_compute(m_group_count[0], m_group_count[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

template <typename T>
DftProgramCopyOutput<T>::DftProgramCopyOutput(vec2i group_size, int n1, int n2)
        : m_group_count(group_count(n1, n2, group_size)),
          m_copy_output(opengl::ComputeShader(copy_output_source<T>(group_size))),
          m_shader_memory(sizeof(ShaderMemory))
{
}

template <typename T>
void DftProgramCopyOutput<T>::copy(T to_mul, const GLuint64 tex, const opengl::StorageBuffer& data)
{
        ShaderMemory m;
        m.to_mul = to_mul;
        m_shader_memory.copy(m);

        m_copy_output.set_uniform_handle(DST_IMAGE_LOCATION, tex);
        m_shader_memory.bind(DATA_BINDING);
        data.bind(BUFFER_BINDING);

        m_copy_output.dispatch_compute(m_group_count[0], m_group_count[1], 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

//

template <typename T>
DftProgramMul<T>::DftProgramMul(vec2i group_size, int n1, int n2, int m1, int m2)
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
void DftProgramMul<T>::set_and_bind(bool inverse, const opengl::StorageBuffer& data, const opengl::StorageBuffer& buffer) const
{
        ShaderMemory m;
        m.inverse_dft = inverse;
        m_shader_memory.copy(m);

        m_shader_memory.bind(DATA_BINDING);
        data.bind(BUFFER_0_BINDING);
        buffer.bind(BUFFER_1_BINDING);
}

template <typename T>
void DftProgramMul<T>::rows_to_buffer(bool inverse, const opengl::StorageBuffer& data, const opengl::StorageBuffer& buffer) const
{
        set_and_bind(inverse, data, buffer);

        m_rows_to_buffer.dispatch_compute(m_rows_to_buffer_groups[0], m_rows_to_buffer_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

template <typename T>
void DftProgramMul<T>::rows_from_buffer(bool inverse, const opengl::StorageBuffer& data,
                                        const opengl::StorageBuffer& buffer) const
{
        set_and_bind(inverse, data, buffer);

        m_rows_from_buffer.dispatch_compute(m_rows_from_buffer_groups[0], m_rows_from_buffer_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

template <typename T>
void DftProgramMul<T>::columns_to_buffer(bool inverse, const opengl::StorageBuffer& data,
                                         const opengl::StorageBuffer& buffer) const
{
        set_and_bind(inverse, data, buffer);

        m_columns_to_buffer.dispatch_compute(m_columns_to_buffer_groups[0], m_columns_to_buffer_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

template <typename T>
void DftProgramMul<T>::columns_from_buffer(bool inverse, const opengl::StorageBuffer& data,
                                           const opengl::StorageBuffer& buffer) const
{
        set_and_bind(inverse, data, buffer);

        m_columns_from_buffer.dispatch_compute(m_columns_from_buffer_groups[0], m_columns_from_buffer_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

template <typename T>
DftProgramMulD<T>::DftProgramMulD(vec2i group_size, int n1, int n2, int m1, int m2)
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
void DftProgramMulD<T>::rows_mul_d(const opengl::StorageBuffer& d, const opengl::StorageBuffer& data) const
{
        m_memory_rows.bind(DATA_BINDING);
        d.bind(BUFFER_DIAGONAL_BINDING);
        data.bind(BUFFER_DATA_BINDING);

        m_mul_d.dispatch_compute(m_row_groups[0], m_row_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

template <typename T>
void DftProgramMulD<T>::columns_mul_d(const opengl::StorageBuffer& d, const opengl::StorageBuffer& data) const
{
        m_memory_columns.bind(DATA_BINDING);
        d.bind(BUFFER_DIAGONAL_BINDING);
        data.bind(BUFFER_DATA_BINDING);

        m_mul_d.dispatch_compute(m_column_groups[0], m_column_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

template <typename T>
DftProgramFftShared<T>::DftProgramFftShared(int n, int shared_size, int group_size, bool reverse_input)
        : m_n(n),
          m_n_bits(binary_size(n)),
          m_shared_size(shared_size),
          m_fft(opengl::ComputeShader(fft_shared_source<T>(m_n, m_n_bits, m_shared_size, group_size, reverse_input))),
          m_shader_memory(sizeof(ShaderMemory))
{
        ASSERT((1 << m_n_bits) == m_n);
}

template <typename T>
void DftProgramFftShared<T>::exec(bool inverse, int data_size, const opengl::StorageBuffer& data) const
{
        ShaderMemory m;
        m.inverse_dft = inverse;
        m.data_size = data_size;
        m_shader_memory.copy(m);

        m_shader_memory.bind(DATA_BINDING);
        data.bind(BUFFER_BINDING);

        m_fft.dispatch_compute(group_count(data_size, m_shared_size), 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

template class DftProgramBitReverse<float>;
template class DftProgramFftGlobal<float>;
template class DftProgramCopyInput<float>;
template class DftProgramCopyOutput<float>;
template class DftProgramMul<float>;
template class DftProgramMulD<float>;
template class DftProgramFftShared<float>;
}
