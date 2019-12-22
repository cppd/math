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

#pragma once

#if defined(OPENGL_FOUND)

#include "com/color/color.h"
#include "com/matrix.h"
#include "com/vec.h"
#include "graphics/opengl/buffers.h"

namespace gpu_opengl
{
class TextShaderMemory final
{
        static constexpr int MATRICES_BINDING = 0;
        static constexpr int DRAWING_BINDING = 2;

        struct Matrices
        {
                Matrix<4, 4, float> matrix;
        };

        struct Drawing
        {
                vec3f text_color;
        };

        opengl::Buffer m_matrices;
        opengl::Buffer m_drawing;

public:
        TextShaderMemory() : m_matrices(sizeof(Matrices), GL_MAP_WRITE_BIT), m_drawing(sizeof(Drawing), GL_MAP_WRITE_BIT)
        {
        }

        void set_matrix(const mat4& matrix) const
        {
                decltype(Matrices().matrix) m = transpose(to_matrix<float>(matrix));
                opengl::map_and_write_to_buffer(m_matrices, offsetof(Matrices, matrix), m);
        }

        void set_color(const Color& color) const
        {
                decltype(Drawing().text_color) c = color.to_rgb_vector<float>();
                opengl::map_and_write_to_buffer(m_drawing, offsetof(Drawing, text_color), c);
        }

        void bind() const
        {
                glBindBufferBase(GL_UNIFORM_BUFFER, MATRICES_BINDING, m_matrices);
                glBindBufferBase(GL_UNIFORM_BUFFER, DRAWING_BINDING, m_drawing);
        }
};
}

#endif
