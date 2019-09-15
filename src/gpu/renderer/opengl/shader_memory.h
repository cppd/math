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
#include "gpu/com/glsl.h"
#include "graphics/opengl/buffers.h"

namespace gpu_opengl
{
class RendererTrianglesMemory final
{
        static constexpr int MATRICES_BINDING = 0;
        static constexpr int LIGHTING_BINDING = 1;
        static constexpr int DRAWING_BINDING = 2;

        opengl::Buffer m_matrices;
        opengl::Buffer m_lighting;
        opengl::Buffer m_drawing;

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
        RendererTrianglesMemory()
                : m_matrices(sizeof(Matrices), GL_MAP_WRITE_BIT),
                  m_lighting(sizeof(Lighting), GL_MAP_WRITE_BIT),
                  m_drawing(sizeof(Drawing), GL_MAP_WRITE_BIT)
        {
        }

        void bind() const
        {
                glBindBufferBase(GL_UNIFORM_BUFFER, MATRICES_BINDING, m_matrices);
                glBindBufferBase(GL_UNIFORM_BUFFER, LIGHTING_BINDING, m_lighting);
                glBindBufferBase(GL_UNIFORM_BUFFER, DRAWING_BINDING, m_drawing);
        }

        void set_matrices(const mat4& matrix, const mat4& shadow_matrix) const
        {
                Matrices matrices;
                matrices.matrix = transpose(to_matrix<float>(matrix));
                matrices.shadow_matrix = transpose(to_matrix<float>(shadow_matrix));
                opengl::map_and_write_to_buffer(m_matrices, matrices);
        }

        void set_direction_to_light(const vec3& direction) const
        {
                decltype(Lighting().direction_to_light) d = to_vector<float>(direction);
                opengl::map_and_write_to_buffer(m_lighting, offsetof(Lighting, direction_to_light), d);
        }

        void set_direction_to_camera(const vec3& direction) const
        {
                decltype(Lighting().direction_to_camera) d = to_vector<float>(direction);
                opengl::map_and_write_to_buffer(m_lighting, offsetof(Lighting, direction_to_camera), d);
        }

        void set_show_smooth(bool show) const
        {
                decltype(Lighting().show_smooth) s = show ? 1 : 0;
                opengl::map_and_write_to_buffer(m_lighting, offsetof(Lighting, show_smooth), s);
        }

        void set_default_color(const Color& color) const
        {
                decltype(Drawing().default_color) c = color.to_rgb_vector<float>();
                opengl::map_and_write_to_buffer(m_drawing, offsetof(Drawing, default_color), c);
        }

        void set_wireframe_color(const Color& color) const
        {
                decltype(Drawing().wireframe_color) c = color.to_rgb_vector<float>();
                opengl::map_and_write_to_buffer(m_drawing, offsetof(Drawing, wireframe_color), c);
        }

        void set_default_ns(float default_ns) const
        {
                decltype(Drawing().default_ns) d = default_ns;
                opengl::map_and_write_to_buffer(m_drawing, offsetof(Drawing, default_ns), d);
        }

        void set_light_a(const Color& color) const
        {
                decltype(Drawing().light_a) c = color.to_rgb_vector<float>();
                opengl::map_and_write_to_buffer(m_drawing, offsetof(Drawing, light_a), c);
        }

        void set_light_d(const Color& color) const
        {
                decltype(Drawing().light_d) c = color.to_rgb_vector<float>();
                opengl::map_and_write_to_buffer(m_drawing, offsetof(Drawing, light_d), c);
        }

        void set_light_s(const Color& color) const
        {
                decltype(Drawing().light_s) c = color.to_rgb_vector<float>();
                opengl::map_and_write_to_buffer(m_drawing, offsetof(Drawing, light_s), c);
        }

        void set_show_materials(bool show) const
        {
                decltype(Drawing().show_materials) s = show ? 1 : 0;
                opengl::map_and_write_to_buffer(m_drawing, offsetof(Drawing, show_materials), s);
        }

        void set_show_wireframe(bool show) const
        {
                decltype(Drawing().show_wireframe) s = show ? 1 : 0;
                opengl::map_and_write_to_buffer(m_drawing, offsetof(Drawing, show_wireframe), s);
        }

        void set_show_shadow(bool show) const
        {
                decltype(Drawing().show_shadow) s = show ? 1 : 0;
                opengl::map_and_write_to_buffer(m_drawing, offsetof(Drawing, show_shadow), s);
        }
};

class RendererMaterialMemory final
{
        static constexpr int MATERIALS_BINDING = 3;

        std::vector<opengl::Buffer> m_materials;

public:
        struct Material
        {
                alignas(GLSL_VEC3_ALIGN) vec3f Ka;
                alignas(GLSL_VEC3_ALIGN) vec3f Kd;
                alignas(GLSL_VEC3_ALIGN) vec3f Ks;
                GLuint64 texture_Ka;
                GLuint64 texture_Kd;
                GLuint64 texture_Ks;
                GLfloat Ns;
                GLuint use_texture_Ka;
                GLuint use_texture_Kd;
                GLuint use_texture_Ks;
                GLuint use_material;
        };

        RendererMaterialMemory(const std::vector<Material>& materials)
        {
                m_materials.reserve(materials.size());
                for (const Material& m : materials)
                {
                        m_materials.emplace_back(sizeof(Material), &m, 0);
                }
        }

        unsigned material_count() const
        {
                return m_materials.size();
        }

        void bind(unsigned index) const
        {
                ASSERT(index < m_materials.size());
                glBindBufferBase(GL_UNIFORM_BUFFER, MATERIALS_BINDING, m_materials[index]);
        }
};

class RendererPointsMemory final
{
        static constexpr int MATRICES_BINDING = 0;
        static constexpr int DRAWING_BINDING = 1;

        opengl::Buffer m_matrices;
        opengl::Buffer m_drawing;

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
        RendererPointsMemory() : m_matrices(sizeof(Matrices), GL_MAP_WRITE_BIT), m_drawing(sizeof(Drawing), GL_MAP_WRITE_BIT)
        {
        }

        void bind() const
        {
                glBindBufferBase(GL_UNIFORM_BUFFER, MATRICES_BINDING, m_matrices);
                glBindBufferBase(GL_UNIFORM_BUFFER, DRAWING_BINDING, m_drawing);
        }

        void set_matrix(const mat4& matrix) const
        {
                decltype(Matrices().matrix) m = transpose(to_matrix<float>(matrix));
                opengl::map_and_write_to_buffer(m_matrices, offsetof(Matrices, matrix), m);
        }

        void set_default_color(const Color& color) const
        {
                decltype(Drawing().default_color) c = color.to_rgb_vector<float>();
                opengl::map_and_write_to_buffer(m_drawing, offsetof(Drawing, default_color), c);
        }

        void set_background_color(const Color& color) const
        {
                decltype(Drawing().background_color) c = color.to_rgb_vector<float>();
                opengl::map_and_write_to_buffer(m_drawing, offsetof(Drawing, background_color), c);
        }

        void set_light_a(const Color& color) const
        {
                decltype(Drawing().light_a) c = color.to_rgb_vector<float>();
                opengl::map_and_write_to_buffer(m_drawing, offsetof(Drawing, light_a), c);
        }

        void set_show_fog(bool show) const
        {
                decltype(Drawing().show_fog) s = show ? 1 : 0;
                opengl::map_and_write_to_buffer(m_drawing, offsetof(Drawing, show_fog), s);
        }
};

class RendererShadowMemory final
{
        static constexpr int MATRICES_BINDING = 0;

        opengl::Buffer m_matrices;

        struct Matrices
        {
                Matrix<4, 4, float> matrix;
        };

public:
        RendererShadowMemory() : m_matrices(sizeof(Matrices), GL_MAP_WRITE_BIT)
        {
        }

        void bind() const
        {
                glBindBufferBase(GL_UNIFORM_BUFFER, MATRICES_BINDING, m_matrices);
        }

        void set_matrix(const mat4& matrix) const
        {
                decltype(Matrices().matrix) m = transpose(to_matrix<float>(matrix));
                opengl::map_and_write_to_buffer(m_matrices, offsetof(Matrices, matrix), m);
        }
};
}
