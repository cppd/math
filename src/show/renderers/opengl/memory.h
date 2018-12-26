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
#include "com/matrix.h"
#include "com/vec.h"
#include "graphics/glsl.h"
#include "graphics/opengl/buffers.h"

namespace opengl_renderer_shaders
{
class TrianglesMemory
{
        static constexpr int MATRICES_BINDING = 0;
        static constexpr int LIGHTING_BINDING = 1;
        static constexpr int DRAWING_BINDING = 2;

        opengl::UniformBuffer m_matrices;
        opengl::UniformBuffer m_lighting;
        opengl::UniformBuffer m_drawing;

        struct Matrices
        {
                Matrix<4, 4, float> matrix;
                Matrix<4, 4, float> shadow_matrix;
        };

        struct Lighting
        {
                alignas(GLSL_VEC3_ALIGN) vec3f direction_to_light;
                alignas(GLSL_VEC3_ALIGN) vec3f direction_to_camera;
                GLuint show_smooth;
        };

        struct Drawing
        {
                alignas(GLSL_VEC3_ALIGN) vec3f default_color;
                alignas(GLSL_VEC3_ALIGN) vec3f wireframe_color;
                float default_ns;
                alignas(GLSL_VEC3_ALIGN) vec3f light_a;
                alignas(GLSL_VEC3_ALIGN) vec3f light_d;
                alignas(GLSL_VEC3_ALIGN) vec3f light_s;
                GLuint show_materials;
                GLuint show_wireframe;
                GLuint show_shadow;
        };

public:
        TrianglesMemory() : m_matrices(sizeof(Matrices)), m_lighting(sizeof(Lighting)), m_drawing(sizeof(Drawing))
        {
        }

        void bind() const
        {
                m_matrices.bind(MATRICES_BINDING);
                m_lighting.bind(LIGHTING_BINDING);
                m_drawing.bind(DRAWING_BINDING);
        }

        void set_matrix(const mat4& matrix) const
        {
                decltype(Matrices().matrix) m = transpose(to_matrix<float>(matrix));
                m_matrices.copy(offsetof(Matrices, matrix), m);
        }
        void set_shadow_matrix(const mat4& matrix) const
        {
                decltype(Matrices().shadow_matrix) m = transpose(to_matrix<float>(matrix));
                m_matrices.copy(offsetof(Matrices, shadow_matrix), m);
        }

        void set_direction_to_light(const vec3& direction) const
        {
                decltype(Lighting().direction_to_light) d = to_vector<float>(direction);
                m_lighting.copy(offsetof(Lighting, direction_to_light), d);
        }

        void set_direction_to_camera(const vec3& direction) const
        {
                decltype(Lighting().direction_to_camera) d = to_vector<float>(direction);
                m_lighting.copy(offsetof(Lighting, direction_to_camera), d);
        }

        void set_show_smooth(bool show) const
        {
                decltype(Lighting().show_smooth) s = show ? 1 : 0;
                m_lighting.copy(offsetof(Lighting, show_smooth), s);
        }

        void set_default_color(const Color& color) const
        {
                decltype(Drawing().default_color) c = color.to_rgb_vector<float>();
                m_drawing.copy(offsetof(Drawing, default_color), c);
        }

        void set_wireframe_color(const Color& color) const
        {
                decltype(Drawing().wireframe_color) c = color.to_rgb_vector<float>();
                m_drawing.copy(offsetof(Drawing, wireframe_color), c);
        }

        void set_default_ns(float default_ns) const
        {
                decltype(Drawing().default_ns) d = default_ns;
                m_drawing.copy(offsetof(Drawing, default_ns), d);
        }

        void set_light_a(const Color& color) const
        {
                decltype(Drawing().light_a) c = color.to_rgb_vector<float>();
                m_drawing.copy(offsetof(Drawing, light_a), c);
        }

        void set_light_d(const Color& color) const
        {
                decltype(Drawing().light_d) c = color.to_rgb_vector<float>();
                m_drawing.copy(offsetof(Drawing, light_d), c);
        }

        void set_light_s(const Color& color) const
        {
                decltype(Drawing().light_s) c = color.to_rgb_vector<float>();
                m_drawing.copy(offsetof(Drawing, light_s), c);
        }

        void set_show_materials(bool show) const
        {
                decltype(Drawing().show_materials) s = show ? 1 : 0;
                m_drawing.copy(offsetof(Drawing, show_materials), s);
        }

        void set_show_wireframe(bool show) const
        {
                decltype(Drawing().show_wireframe) s = show ? 1 : 0;
                m_drawing.copy(offsetof(Drawing, show_wireframe), s);
        }

        void set_show_shadow(bool show) const
        {
                decltype(Drawing().show_shadow) s = show ? 1 : 0;
                m_drawing.copy(offsetof(Drawing, show_shadow), s);
        }
};

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

class ShadowMemory
{
        static constexpr int MATRICES_BINDING = 0;

        opengl::UniformBuffer m_matrices;

        struct Matrices
        {
                Matrix<4, 4, float> matrix;
        };

public:
        ShadowMemory() : m_matrices(sizeof(Matrices))
        {
        }

        void bind() const
        {
                m_matrices.bind(MATRICES_BINDING);
        }

        void set_matrix(const mat4& matrix) const
        {
                decltype(Matrices().matrix) m = transpose(to_matrix<float>(matrix));
                m_matrices.copy(offsetof(Matrices, matrix), m);
        }
};
}
