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

#include "com/color/color.h"
#include "com/matrix.h"
#include "com/vec.h"
#include "graphics/opengl/buffers.h"

namespace opengl_text_implementation
{
class ShaderMemory
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

        opengl::UniformBuffer m_matrices;
        opengl::UniformBuffer m_drawing;

public:
        ShaderMemory() : m_matrices(sizeof(Matrices)), m_drawing(sizeof(Drawing))
        {
        }

        void set_matrix(const mat4& matrix) const
        {
                decltype(Matrices().matrix) m = transpose(to_matrix<float>(matrix));
                m_matrices.copy(offsetof(Matrices, matrix), m);
        }

        void set_color(const Color& color) const
        {
                decltype(Drawing().text_color) c = color.to_rgb_vector<float>();
                m_drawing.copy(offsetof(Drawing, text_color), c);
        }

        void bind() const
        {
                m_matrices.bind(MATRICES_BINDING);
                m_drawing.bind(DRAWING_BINDING);
        }
};
}
