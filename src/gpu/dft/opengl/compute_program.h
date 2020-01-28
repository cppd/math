/*
Copyright (C) 2017-2020 Topological Manifold

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

#pragma once

#if defined(OPENGL_FOUND)

#include "com/vec.h"
#include "graphics/opengl/buffers.h"
#include "graphics/opengl/shader.h"

namespace gpu_opengl
{
class DftProgramBitReverse final
{
        static constexpr int BUFFER_BINDING = 0;

        int m_group_count;

        opengl::ComputeProgram m_bit_reverse;

public:
        DftProgramBitReverse(int group_size, int count, int n);

        void exec(const opengl::Buffer& data) const;
};

class DftProgramFftGlobal final
{
        const int m_count, m_n;
        const int m_group_count;
        opengl::ComputeProgram m_fft_forward;
        opengl::ComputeProgram m_fft_inverse;

public:
        DftProgramFftGlobal(int count, int n, int group_size);
        void exec(bool inverse) const;
};

class DftProgramCopyInput final
{
        static constexpr int SRC_LOCATION = 0;
        static constexpr int DST_BINDING = 0;

        const vec2i m_group_count;
        opengl::ComputeProgram m_program;

public:
        DftProgramCopyInput(
                vec2i group_size,
                const opengl::Texture& texture,
                unsigned x,
                unsigned y,
                unsigned width,
                unsigned height);

        void copy(const opengl::Buffer& data);
};

class DftProgramCopyOutput final
{
        static constexpr int SRC_BINDING = 0;
        static constexpr int DST_LOCATION = 0;

        const vec2i m_group_count;
        opengl::ComputeProgram m_program;

public:
        DftProgramCopyOutput(vec2i group_size, const opengl::Texture& texture, int n1, int n2, float to_mul);

        void copy(const opengl::Buffer& data);
};

class DftProgramMul final
{
        static constexpr int DATA_BINDING = 0;
        static constexpr int BUFFER_BINDING = 1;

        const vec2i m_rows_to_buffer_groups;
        const vec2i m_rows_from_buffer_groups;
        const vec2i m_columns_to_buffer_groups;
        const vec2i m_columns_from_buffer_groups;
        opengl::ComputeProgram m_rows_to_buffer_forward;
        opengl::ComputeProgram m_rows_to_buffer_inverse;
        opengl::ComputeProgram m_rows_from_buffer_forward;
        opengl::ComputeProgram m_rows_from_buffer_inverse;
        opengl::ComputeProgram m_columns_to_buffer_forward;
        opengl::ComputeProgram m_columns_to_buffer_inverse;
        opengl::ComputeProgram m_columns_from_buffer_forward;
        opengl::ComputeProgram m_columns_from_buffer_inverse;

        void bind(const opengl::Buffer& data, const opengl::Buffer& buffer) const;

public:
        DftProgramMul(vec2i group_size, int n1, int n2, int m1, int m2z);

        // Функции подстановки переменных, формулы 13.4, 13.27, 13.28, 13.32.
        void rows_to_buffer(bool inverse, const opengl::Buffer& data, const opengl::Buffer& buffer) const;
        void rows_from_buffer(bool inverse, const opengl::Buffer& data, const opengl::Buffer& buffer) const;
        void columns_to_buffer(bool inverse, const opengl::Buffer& data, const opengl::Buffer& buffer) const;
        void columns_from_buffer(bool inverse, const opengl::Buffer& data, const opengl::Buffer& buffer) const;
};

class DftProgramMulD final
{
        static constexpr int DIAGONAL_BINDING = 0;
        static constexpr int DATA_BINDING = 1;

        const vec2i m_row_groups, m_column_groups;
        opengl::ComputeProgram m_mul_d_rows;
        opengl::ComputeProgram m_mul_d_columns;

public:
        DftProgramMulD(vec2i group_size, int n1, int n2, int m1, int m2);

        // Умножение на диагональ, формулы 13.20, 13.30.
        void rows_mul_d(const opengl::Buffer& d, const opengl::Buffer& data) const;
        void columns_mul_d(const opengl::Buffer& d, const opengl::Buffer& data) const;
};

class DftProgramFftShared final
{
        static constexpr int BUFFER_BINDING = 0;

        const int m_count, m_n, m_n_bits, m_shared_size;
        const int m_group_count;
        opengl::ComputeProgram m_fft_forward;
        opengl::ComputeProgram m_fft_inverse;

public:
        DftProgramFftShared(int count, int n, int shared_size, int group_size, bool reverse_input);
        void exec(bool inverse, const opengl::Buffer& data) const;
};
}

#endif
