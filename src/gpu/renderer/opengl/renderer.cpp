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

#include "renderer.h"

#include "draw_object.h"
#include "shader_memory.h"
#include "shader_source.h"

#include "com/log.h"
#include "com/matrix_alg.h"
#include "com/print.h"
#include "gpu/renderer/com/storage.h"
#include "graphics/opengl/capabilities.h"
#include "graphics/opengl/query.h"
#include "graphics/opengl/shader.h"

#include <cmath>
#include <string>

constexpr GLenum DEPTH_FORMAT = GL_DEPTH_COMPONENT32;

namespace gpu_opengl
{
namespace
{
class Impl final : public Renderer
{
        static constexpr mat4 SCALE = scale<double>(0.5, 0.5, 0.5);
        static constexpr mat4 TRANSLATE = translate<double>(1, 1, 1);
        const mat4 SCALE_BIAS_MATRIX = SCALE * TRANSLATE;

        opengl::GraphicsProgram m_triangles_program;
        opengl::GraphicsProgram m_shadow_program;
        opengl::GraphicsProgram m_points_0d_program;
        opengl::GraphicsProgram m_points_1d_program;

        std::unique_ptr<opengl::DepthFramebuffer> m_shadow_buffer;

        mat4 m_shadow_matrix;
        mat4 m_scale_bias_shadow_matrix;
        mat4 m_main_matrix;

        bool m_show_shadow = false;

        int m_x = -1;
        int m_y = -1;
        int m_width = -1;
        int m_height = -1;
        int m_shadow_width = -1;
        int m_shadow_height = -1;

        const int m_max_texture_size = opengl::max_texture_size();

        double m_shadow_zoom = 1;

        RendererObjectStorage<DrawObject> m_storage;

        RendererPointsMemory m_points_memory;
        RendererShadowMemory m_shadow_memory;
        RendererTrianglesMemory m_triangles_memory;

        const opengl::Texture* m_object_image = nullptr;

        void set_light_a(const Color& light) override
        {
                m_triangles_memory.set_light_a(light);
                m_points_memory.set_light_a(light);
        }
        void set_light_d(const Color& light) override
        {
                m_triangles_memory.set_light_d(light);
        }
        void set_light_s(const Color& light) override
        {
                m_triangles_memory.set_light_s(light);
        }
        void set_background_color(const Color& color) override
        {
                glClearColor(color.red(), color.green(), color.blue(), 1);
                m_points_memory.set_background_color(color);
        }
        void set_default_color(const Color& color) override
        {
                m_triangles_memory.set_default_color(color);
                m_points_memory.set_default_color(color);
        }
        void set_wireframe_color(const Color& color) override
        {
                m_triangles_memory.set_wireframe_color(color);
        }
        void set_default_ns(double default_ns) override
        {
                m_triangles_memory.set_default_ns(default_ns);
        }
        void set_show_smooth(bool show) override
        {
                m_triangles_memory.set_show_smooth(show);
        }
        void set_show_wireframe(bool show) override
        {
                m_triangles_memory.set_show_wireframe(show);
        }
        void set_show_shadow(bool show) override
        {
                m_show_shadow = show;
                m_triangles_memory.set_show_shadow(show);
        }
        void set_show_fog(bool show) override
        {
                m_points_memory.set_show_fog(show);
        }
        void set_show_materials(bool show) override
        {
                m_triangles_memory.set_show_materials(show);
        }
        void set_camera(const RendererCameraInfo& c) override
        {
                const mat4& shadow_projection_matrix =
                        ortho_opengl<double>(c.shadow_volume.left, c.shadow_volume.right, c.shadow_volume.bottom,
                                             c.shadow_volume.top, c.shadow_volume.near, c.shadow_volume.far);
                const mat4& view_projection_matrix =
                        ortho_opengl<double>(c.view_volume.left, c.view_volume.right, c.view_volume.bottom, c.view_volume.top,
                                             c.view_volume.near, c.view_volume.far);

                m_shadow_matrix = shadow_projection_matrix * c.shadow_matrix;
                m_scale_bias_shadow_matrix = SCALE_BIAS_MATRIX * m_shadow_matrix;
                m_main_matrix = view_projection_matrix * c.view_matrix;

                m_triangles_memory.set_direction_to_light(-c.light_direction);
                m_triangles_memory.set_direction_to_camera(-c.camera_direction);

                set_matrices();
        }

        void draw() override
        {
                const DrawObject* draw_object = m_storage.object();

                ASSERT(m_object_image);

                m_object_image->clear();

                if (!draw_object)
                {
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                        return;
                }

                opengl::GLEnableAndRestore<GL_DEPTH_TEST> enable_depth_test;

                if (m_show_shadow && draw_object->has_shadow())
                {
                        opengl::FramebufferBinder binder(*m_shadow_buffer);

                        glViewport(0, 0, m_shadow_width, m_shadow_height);
                        glClearDepthf(1.0f);
                        glClear(GL_DEPTH_BUFFER_BIT);

                        // depth-fighting
                        opengl::GLEnableAndRestore<GL_POLYGON_OFFSET_FILL> enable_polygon_offset_fill;
                        glPolygonOffset(2.0f, 2.0f); // glPolygonOffset(4.0f, 4.0f);

                        ShadowInfo info;
                        info.triangles_program = &m_shadow_program;
                        info.triangles_memory = &m_shadow_memory;
                        draw_object->shadow(info);
                }

                glViewport(m_x, m_y, m_width, m_height);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                DrawInfo info;
                info.triangles_program = &m_triangles_program;
                info.triangles_memory = &m_triangles_memory;
                info.points_program = &m_points_0d_program;
                info.points_memory = &m_points_memory;
                info.lines_program = &m_points_1d_program;
                info.lines_memory = &m_points_memory;
                draw_object->draw(info);
        }

        virtual bool empty() const override
        {
                return !m_storage.object();
        }

        void set_shadow_size()
        {
                if (m_width <= 0 || m_height <= 0)
                {
                        return;
                }

                m_shadow_width = std::lround(m_shadow_zoom * m_width);
                m_shadow_height = std::lround(m_shadow_zoom * m_height);

                if (m_shadow_width > m_max_texture_size)
                {
                        LOG("Shadow texture width is too big " + to_string(m_shadow_width) + ", set to max " +
                            to_string(m_max_texture_size));
                        m_shadow_width = m_max_texture_size;
                }
                if (m_shadow_width <= 0)
                {
                        LOG("Shadow texture width is 0 , set to 1");
                        m_shadow_width = 1;
                }
                if (m_shadow_height > m_max_texture_size)
                {
                        LOG("Shadow texture height is too big " + to_string(m_shadow_height) + ", set to max " +
                            to_string(m_max_texture_size));
                        m_shadow_height = m_max_texture_size;
                }
                if (m_shadow_height <= 0)
                {
                        LOG("Shadow texture height is 0 , set to 1");
                        m_shadow_height = 1;
                }

                m_shadow_buffer = std::make_unique<opengl::DepthFramebuffer>(DEPTH_FORMAT, m_shadow_width, m_shadow_height);
                m_triangles_program.set_uniform_handle("shadow_texture", m_shadow_buffer->texture().texture_handle());
        }

        void set_matrices()
        {
                ASSERT(m_storage.scale_object() || !m_storage.object());

                if (m_storage.scale_object())
                {
                        mat4 matrix = m_main_matrix * m_storage.scale_object()->model_matrix();
                        mat4 scale_bias_shadow_matrix = m_scale_bias_shadow_matrix * m_storage.scale_object()->model_matrix();
                        mat4 shadow_matrix = m_shadow_matrix * m_storage.scale_object()->model_matrix();

                        m_triangles_memory.set_matrices(matrix, scale_bias_shadow_matrix);
                        m_shadow_memory.set_matrix(shadow_matrix);
                        m_points_memory.set_matrix(matrix);
                }
        }

        void set_shadow_zoom(double zoom) override
        {
                m_shadow_zoom = zoom;

                set_shadow_size();
        }

        void set_size(int x, int y, int width, int height, const opengl::Texture& object_image) override
        {
                ASSERT(object_image.format() == GL_R32UI);

                m_x = x;
                m_y = y;
                m_width = width;
                m_height = height;

                m_object_image = &object_image;

                m_triangles_program.set_uniform_handle("object_image", m_object_image->image_handle_write_only());
                m_points_0d_program.set_uniform_handle("object_image", m_object_image->image_handle_write_only());
                m_points_1d_program.set_uniform_handle("object_image", m_object_image->image_handle_write_only());

                set_shadow_size();
        }

        void object_add(const Obj<3>* obj, double size, const vec3& position, int id, int scale_id) override
        {
                m_storage.add_object(std::make_unique<DrawObject>(*obj, size, position), id, scale_id);
                set_matrices();
        }
        void object_delete(int id) override
        {
                m_storage.delete_object(id);
                set_matrices();
        }
        void object_show(int id) override
        {
                m_storage.show_object(id);
                set_matrices();
        }
        void object_delete_all() override
        {
                m_storage.delete_all();
                set_matrices();
        }

public:
        Impl()
                : m_triangles_program(opengl::VertexShader(renderer_triangles_vert()),
                                      opengl::GeometryShader(renderer_triangles_geom()),
                                      opengl::FragmentShader(renderer_triangles_frag())),
                  m_shadow_program(opengl::VertexShader(renderer_shadow_vert()), opengl::FragmentShader(renderer_shadow_frag())),
                  m_points_0d_program(opengl::VertexShader(renderer_points_0d_vert()),
                                      opengl::FragmentShader(renderer_points_frag())),
                  m_points_1d_program(opengl::VertexShader(renderer_points_1d_vert()),
                                      opengl::FragmentShader(renderer_points_frag()))
        {
        }
};
}

std::unique_ptr<Renderer> create_renderer()
{
        return std::make_unique<Impl>();
}
}
