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

#pragma once

#include "com/color/color.h"
#include "com/vec.h"
#include "graphics/glsl.h"
#include "graphics/opengl/objects.h"

namespace opengl_renderer_shaders
{
class PointsMemory
{
        static constexpr int MATRICES_BINDING = 0;
        static constexpr int DRAWING_BINDING = 1;

        opengl::UniformBuffer m_matrices;
        opengl::UniformBuffer m_drawing;

        struct Matrices
        {
                Matrix<4, 4, float> matrix;
        };

        struct Drawing
        {
                alignas(GLSL_VEC3_ALIGN) vec3f default_color;
                alignas(GLSL_VEC3_ALIGN) vec3f background_color;
                alignas(GLSL_VEC3_ALIGN) vec3f light_a;
                GLuint show_fog;
        };

public:
        PointsMemory() : m_matrices(sizeof(Matrices)), m_drawing(sizeof(Drawing))
        {
        }

        void bind() const
        {
                m_matrices.bind(MATRICES_BINDING);
                m_drawing.bind(DRAWING_BINDING);
        }

        void set_matrix(const mat4& matrix) const
        {
                decltype(Matrices().matrix) m = transpose(to_matrix<float>(matrix));
                m_matrices.copy(offsetof(Matrices, matrix), m);
        }

        void set_default_color(const Color& color) const
        {
                decltype(Drawing().default_color) c = color.to_rgb_vector<float>();
                m_drawing.copy(offsetof(Drawing, default_color), c);
        }

        void set_background_color(const Color& color) const
        {
                decltype(Drawing().background_color) c = color.to_rgb_vector<float>();
                m_drawing.copy(offsetof(Drawing, background_color), c);
        }

        void set_light_a(const Color& color) const
        {
                decltype(Drawing().light_a) c = color.to_rgb_vector<float>();
                m_drawing.copy(offsetof(Drawing, light_a), c);
        }

        void set_show_fog(bool show) const
        {
                decltype(Drawing().show_fog) s = show ? 1 : 0;
                m_drawing.copy(offsetof(Drawing, show_fog), s);
        }
};
}
