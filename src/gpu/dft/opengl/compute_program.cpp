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

std::string bool_string(bool v)
{
        return v ? "true" : "false";
}

std::string bit_reverse_source(int group_size, unsigned count, unsigned n)
{
        std::string s;
        s += group_size_string(group_size);
        s += "const uint DATA_SIZE = " + to_string(count * n) + ";\n";
        s += "const uint N_MASK = " + to_string(n - 1) + ";\n";
        s += "const uint N_BITS = " + to_string(binary_size(n)) + ";\n";
        return dft_bit_reverse_comp(s);
}

std::string fft_global_source(int data_size, int n, int group_size, bool inverse)
{
        std::string s;
        s += group_size_string(group_size);
        s += "const bool INVERSE = " + bool_string(inverse) + ";\n";
        s += "const uint DATA_SIZE = " + to_string(data_size) + ";\n";
        s += "const uint N = " + to_string(n) + ";\n";
        return dft_fft_global_comp(s);
}

std::string rows_mul_to_buffer_source(vec2i group_size, int n1, int n2, int m1, int m2, bool inverse)
{
        std::string s;
        s += group_size_string(group_size);
        s += function_index_string(0);
        s += n_m_string(n1, n2, m1, m2);
        s += "const bool INVERSE = " + bool_string(inverse) + ";\n";
        return dft_mul_comp(s);
}

std::string rows_mul_fr_buffer_source(vec2i group_size, int n1, int n2, int m1, int m2, bool inverse)
{
        std::string s;
        s += group_size_string(group_size);
        s += function_index_string(1);
        s += n_m_string(n1, n2, m1, m2);
        s += "const bool INVERSE = " + bool_string(inverse) + ";\n";
        return dft_mul_comp(s);
}

std::string cols_mul_to_buffer_source(vec2i group_size, int n1, int n2, int m1, int m2, bool inverse)
{
        std::string s;
        s += group_size_string(group_size);
        s += function_index_string(2);
        s += n_m_string(n1, n2, m1, m2);
        s += "const bool INVERSE = " + bool_string(inverse) + ";\n";
        return dft_mul_comp(s);
}

std::string cols_mul_fr_buffer_source(vec2i group_size, int n1, int n2, int m1, int m2, bool inverse)
{
        std::string s;
        s += group_size_string(group_size);
        s += function_index_string(3);
        s += n_m_string(n1, n2, m1, m2);
        s += "const bool INVERSE = " + bool_string(inverse) + ";\n";
        return dft_mul_comp(s);
}

std::string rows_mul_d_source(vec2i group_size, int rows, int columns)
{
        std::string s;
        s += group_size_string(group_size);
        s += "const int ROWS = " + to_string(rows) + ";\n";
        s += "const int COLUMNS = " + to_string(columns) + ";\n";
        return dft_mul_d_comp(s);
}

std::string copy_input_source(vec2i group_size, unsigned x, unsigned y, unsigned width, unsigned height)
{
        std::string s;
        s += group_size_string(group_size);
        s += "const int X = " + to_string(x) + ";\n";
        s += "const int Y = " + to_string(y) + ";\n";
        s += "const int WIDTH = " + to_string(width) + ";\n";
        s += "const int HEIGHT = " + to_string(height) + ";\n";
        return dft_copy_input_comp(s);
}

std::string copy_output_source(vec2i group_size, float to_mul)
{
        std::string s;
        s += group_size_string(group_size);
        s += "const float TO_MUL = " + to_string(to_mul) + ";\n";
        return dft_copy_output_comp(s);
}

std::string fft_shared_source(bool inverse, int data_size, int n, int n_bits, int shared_size, int group_size, bool reverse_input)
{
        std::string s;
        s += "const bool INVERSE = " + bool_string(inverse) + ";\n";
        s += "const uint DATA_SIZE = " + to_string(data_size) + ";\n";
        s += "const uint N = " + to_string(n) + ";\n";
        s += "const uint N_MASK = " + to_string(n - 1) + ";\n";
        s += "const uint N_BITS = " + to_string(n_bits) + ";\n";
        s += "const uint SHARED_SIZE = " + to_string(shared_size) + ";\n";
        s += "const bool REVERSE_INPUT = " + bool_string(reverse_input) + ";\n";
        s += "const uint GROUP_SIZE = " + to_string(group_size) + ";\n";
        return dft_fft_shared_comp(s);
}
}

//

DftProgramBitReverse::DftProgramBitReverse(int group_size, int count, int n)
        : m_bit_reverse(opengl::ComputeShader(bit_reverse_source(group_size, count, n)))
{
        m_group_count = group_count(count * n, group_size);
}

void DftProgramBitReverse::exec(const opengl::Buffer& data) const
{
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING, data);

        m_bit_reverse.dispatch_compute(m_group_count, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

DftProgramFftGlobal::DftProgramFftGlobal(int count, int n, int group_size)
        : m_count(count),
          m_n(n),
          m_group_count(group_count((count * n) / 2, group_size)),
          m_fft_forward(opengl::ComputeShader(fft_global_source(m_count * m_n, m_n, group_size, false))),
          m_fft_inverse(opengl::ComputeShader(fft_global_source(m_count * m_n, m_n, group_size, true)))
{
}

void DftProgramFftGlobal::exec(bool inverse) const
{
        if (inverse)
        {
                m_fft_inverse.dispatch_compute(m_group_count, 1, 1);
        }
        else
        {
                m_fft_forward.dispatch_compute(m_group_count, 1, 1);
        }

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

DftProgramCopyInput::DftProgramCopyInput(vec2i group_size, const opengl::Texture& texture, unsigned x, unsigned y, unsigned width,
                                         unsigned height)
        : m_group_count(group_count(width, height, group_size)),
          m_program(opengl::ComputeShader(copy_input_source(group_size, x, y, width, height)))
{
        ASSERT(width > 0 && height > 0);
        ASSERT(x + width <= static_cast<unsigned>(texture.width()));
        ASSERT(y + height <= static_cast<unsigned>(texture.height()));

        m_program.set_uniform_handle(SRC_LOCATION, texture.texture_handle());
}

void DftProgramCopyInput::copy(const opengl::Buffer& data)
{
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DST_BINDING, data);

        m_program.dispatch_compute(m_group_count[0], m_group_count[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

DftProgramCopyOutput::DftProgramCopyOutput(vec2i group_size, const opengl::Texture& texture, int n1, int n2, float to_mul)
        : m_group_count(group_count(n1, n2, group_size)), m_program(opengl::ComputeShader(copy_output_source(group_size, to_mul)))
{
        m_program.set_uniform_handle(DST_LOCATION, texture.image_handle_write_only());
}

void DftProgramCopyOutput::copy(const opengl::Buffer& data)
{
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SRC_BINDING, data);

        m_program.dispatch_compute(m_group_count[0], m_group_count[1], 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

//

DftProgramMul::DftProgramMul(vec2i group_size, int n1, int n2, int m1, int m2)
        : m_rows_to_buffer_groups(group_count(m1, n2, group_size)),
          m_rows_from_buffer_groups(group_count(n1, n2, group_size)),
          m_columns_to_buffer_groups(group_count(n1, m2, group_size)),
          m_columns_from_buffer_groups(group_count(n1, n2, group_size)),
          m_rows_to_buffer_forward(opengl::ComputeShader(rows_mul_to_buffer_source(group_size, n1, n2, m1, m2, false))),
          m_rows_to_buffer_inverse(opengl::ComputeShader(rows_mul_to_buffer_source(group_size, n1, n2, m1, m2, true))),
          m_rows_from_buffer_forward(opengl::ComputeShader(rows_mul_fr_buffer_source(group_size, n1, n2, m1, m2, false))),
          m_rows_from_buffer_inverse(opengl::ComputeShader(rows_mul_fr_buffer_source(group_size, n1, n2, m1, m2, true))),
          m_columns_to_buffer_forward(opengl::ComputeShader(cols_mul_to_buffer_source(group_size, n1, n2, m1, m2, false))),
          m_columns_to_buffer_inverse(opengl::ComputeShader(cols_mul_to_buffer_source(group_size, n1, n2, m1, m2, true))),
          m_columns_from_buffer_forward(opengl::ComputeShader(cols_mul_fr_buffer_source(group_size, n1, n2, m1, m2, false))),
          m_columns_from_buffer_inverse(opengl::ComputeShader(cols_mul_fr_buffer_source(group_size, n1, n2, m1, m2, true)))
{
}

void DftProgramMul::bind(const opengl::Buffer& data, const opengl::Buffer& buffer) const
{
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DATA_BINDING, data);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING, buffer);
}

void DftProgramMul::rows_to_buffer(bool inverse, const opengl::Buffer& data, const opengl::Buffer& buffer) const
{
        bind(data, buffer);

        const opengl::ComputeProgram* p = inverse ? &m_rows_to_buffer_inverse : &m_rows_to_buffer_forward;
        p->dispatch_compute(m_rows_to_buffer_groups[0], m_rows_to_buffer_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void DftProgramMul::rows_from_buffer(bool inverse, const opengl::Buffer& data, const opengl::Buffer& buffer) const
{
        bind(data, buffer);

        const opengl::ComputeProgram* p = inverse ? &m_rows_from_buffer_inverse : &m_rows_from_buffer_forward;
        p->dispatch_compute(m_rows_from_buffer_groups[0], m_rows_from_buffer_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void DftProgramMul::columns_to_buffer(bool inverse, const opengl::Buffer& data, const opengl::Buffer& buffer) const
{
        bind(data, buffer);

        const opengl::ComputeProgram* p = inverse ? &m_columns_to_buffer_inverse : &m_columns_to_buffer_forward;
        p->dispatch_compute(m_columns_to_buffer_groups[0], m_columns_to_buffer_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void DftProgramMul::columns_from_buffer(bool inverse, const opengl::Buffer& data, const opengl::Buffer& buffer) const
{
        bind(data, buffer);

        const opengl::ComputeProgram* p = inverse ? &m_columns_from_buffer_inverse : &m_columns_from_buffer_forward;
        p->dispatch_compute(m_columns_from_buffer_groups[0], m_columns_from_buffer_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

DftProgramMulD::DftProgramMulD(vec2i group_size, int n1, int n2, int m1, int m2)
        : m_row_groups(group_count(m1, n2, group_size)),
          m_column_groups(group_count(m2, n1, group_size)),
          m_mul_d_rows(opengl::ComputeShader(rows_mul_d_source(group_size, n2, m1))),
          m_mul_d_columns(opengl::ComputeShader(rows_mul_d_source(group_size, n1, m2)))
{
}

void DftProgramMulD::rows_mul_d(const opengl::Buffer& d, const opengl::Buffer& data) const
{
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DIAGONAL_BINDING, d);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DATA_BINDING, data);

        m_mul_d_rows.dispatch_compute(m_row_groups[0], m_row_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void DftProgramMulD::columns_mul_d(const opengl::Buffer& d, const opengl::Buffer& data) const
{
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DIAGONAL_BINDING, d);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DATA_BINDING, data);

        m_mul_d_columns.dispatch_compute(m_column_groups[0], m_column_groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

//

DftProgramFftShared::DftProgramFftShared(int count, int n, int shared_size, int group_size, bool reverse_input)
        : m_count(count),
          m_n(n),
          m_n_bits(binary_size(n)),
          m_shared_size(shared_size),
          m_group_count(group_count(m_count * m_n, m_shared_size)),
          m_fft_forward(opengl::ComputeShader(
                  fft_shared_source(false, m_count * m_n, m_n, m_n_bits, m_shared_size, group_size, reverse_input))),
          m_fft_inverse(opengl::ComputeShader(
                  fft_shared_source(true, m_count * m_n, m_n, m_n_bits, m_shared_size, group_size, reverse_input)))
{
        ASSERT((1 << m_n_bits) == m_n);
}

void DftProgramFftShared::exec(bool inverse, const opengl::Buffer& data) const
{
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BUFFER_BINDING, data);
        if (inverse)
        {
                m_fft_inverse.dispatch_compute(m_group_count, 1, 1);
        }
        else
        {
                m_fft_forward.dispatch_compute(m_group_count, 1, 1);
        }
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}
}

#endif
