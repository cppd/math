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

#include "renderer.h"

#include "memory.h"

#include "com/log.h"
#include "com/matrix_alg.h"
#include "com/print.h"
#include "graphics/glsl.h"
#include "graphics/opengl/capabilities.h"
#include "graphics/opengl/query.h"
#include "graphics/opengl/shader.h"
#include "obj/alg/alg.h"
#include "show/renderers/com/storage.h"

#include <algorithm>
#include <vector>

// clang-format off
constexpr const char triangles_vert[]
{
#include "renderer_triangles.vert.str"
};
constexpr const char triangles_geom[]
{
#include "renderer_triangles.geom.str"
};
constexpr const char triangles_frag[]
{
#include "renderer_triangles.frag.str"
};
constexpr const char shadow_vert[]
{
#include "renderer_shadow.vert.str"
};
constexpr const char shadow_frag[]
{
#include "renderer_shadow.frag.str"
};
constexpr const char points_vert[]
{
#include "renderer_points.vert.str"
};
constexpr const char points_frag[]
{
#include "renderer_points.frag.str"
};
// clang-format on

constexpr int BUFFER_BINDING = 3;

namespace
{
enum class DrawType
{
        Points,
        Lines,
        Triangles
};

DrawType draw_type_of_obj(const Obj<3>& obj)
{
        int type_count = 0;

        type_count += obj.facets().size() > 0 ? 1 : 0;
        type_count += obj.points().size() > 0 ? 1 : 0;
        type_count += obj.lines().size() > 0 ? 1 : 0;

        if (type_count > 1)
        {
                error("Supported only faces or points or lines");
        }

        if (obj.facets().size() > 0)
        {
                return DrawType::Triangles;
        }
        else if (obj.points().size() > 0)
        {
                return DrawType::Points;
        }
        else if (obj.lines().size() > 0)
        {
                return DrawType::Lines;
        }
        else
        {
                error("Faces or points or lines not found");
        }
}

// Структуры данных для передачи данных в шейдеры

struct FaceVertex final
{
        vec3f v; // Координаты вершины в пространстве.
        vec3f n; // Нормаль вершины.
        vec2f t; // Координаты вершины в текстуре.
        GLint index; // номер материала.
        // Бит 0: Заданы ли текстурные координаты. Если не заданы текстурные координаты, то использовать цвет материала.
        // Бит 1: Задана ли нормаль. Если не задана нормаль, то использовать одинаковую нормаль для всего треугольника.
        GLubyte property;

        FaceVertex(vec3f v_, vec3f n_, vec2f t_, GLint index_, bool has_tex_coord_, bool has_normal_)
                : v(v_), n(n_), t(t_), index(index_)
        {
                property = 0;
                property |= (has_tex_coord_ ? 0b1 : 0);
                property |= (has_normal_ ? 0b10 : 0);
        }
};

struct PointVertex final
{
        vec3f v; // Координаты вершины в пространстве.

        PointVertex(const vec3f& v_) : v(v_)
        {
        }
};

// shader storage
struct Material final
{
        alignas(GLSL_VEC3_ALIGN) vec3f Ka, Kd, Ks;

        GLuint64 map_Ka_handle, map_Kd_handle, map_Ks_handle;

        GLfloat Ns;

        // если нет текстуры, то -1
        GLint map_Ka, map_Kd, map_Ks;

        explicit Material(const Obj<3>::Material& m)
                : Ka(m.Ka.to_rgb_vector<float>()),
                  Kd(m.Kd.to_rgb_vector<float>()),
                  Ks(m.Ks.to_rgb_vector<float>()),
                  Ns(m.Ns),
                  map_Ka(m.map_Ka),
                  map_Kd(m.map_Kd),
                  map_Ks(m.map_Ks)
        {
        }
};

void load_face_vertices(const Obj<3>& obj, std::vector<FaceVertex>* vertices)
{
        const std::vector<vec3f>& obj_vertices = obj.vertices();
        const std::vector<vec3f>& obj_normals = obj.normals();
        const std::vector<vec2f>& obj_texcoords = obj.texcoords();

        vertices->clear();
        vertices->shrink_to_fit();
        vertices->reserve(obj.facets().size() * 3);

        vec3f v0, v1, v2, n0, n1, n2;
        vec2f t0, t1, t2;

        for (const Obj<3>::Facet& f : obj.facets())
        {
                v0 = obj_vertices[f.vertices[0]];
                v1 = obj_vertices[f.vertices[1]];
                v2 = obj_vertices[f.vertices[2]];

                if (f.has_normal)
                {
                        n0 = obj_normals[f.normals[0]];
                        n1 = obj_normals[f.normals[1]];
                        n2 = obj_normals[f.normals[2]];
                }
                else
                {
                        n0 = n1 = n2 = vec3f(0);
                        // можно один раз вычислять здесь, вместо геометрического шейдера
                        // n0 = n1 = n2 = normalize(cross(v1 - v0, v2 - v0));
                }

                if (f.has_texcoord)
                {
                        t0 = obj_texcoords[f.texcoords[0]];
                        t1 = obj_texcoords[f.texcoords[1]];
                        t2 = obj_texcoords[f.texcoords[2]];
                }
                else
                {
                        t0 = t1 = t2 = vec2f(0);
                }

                vertices->emplace_back(v0, n0, t0, f.material, f.has_texcoord, f.has_normal);
                vertices->emplace_back(v1, n1, t1, f.material, f.has_texcoord, f.has_normal);
                vertices->emplace_back(v2, n2, t2, f.material, f.has_texcoord, f.has_normal);
        }
}

void load_point_vertices(const Obj<3>& obj, std::vector<PointVertex>* vertices)
{
        const std::vector<Obj<3>::Point>& obj_points = obj.points();
        const std::vector<vec3f>& obj_vertices = obj.vertices();

        vertices->clear();
        vertices->shrink_to_fit();
        vertices->reserve(obj_points.size());

        for (const Obj<3>::Point& point : obj_points)
        {
                vertices->emplace_back(obj_vertices[point.vertex]);
        }
}

void load_line_vertices(const Obj<3>& obj, std::vector<PointVertex>* vertices)
{
        const std::vector<Obj<3>::Line>& obj_lines = obj.lines();
        const std::vector<vec3f>& obj_vertices = obj.vertices();

        vertices->clear();
        vertices->shrink_to_fit();
        vertices->reserve(obj_lines.size() * 2);

        for (const Obj<3>::Line& line : obj_lines)
        {
                for (int index : line.vertices)
                {
                        vertices->emplace_back(obj_vertices[index]);
                }
        }
}

void load_materials(const Obj<3>& obj, std::vector<Material>* materials)
{
        const std::vector<Obj<3>::Material>& obj_materials = obj.materials();

        materials->clear();
        materials->shrink_to_fit();
        materials->reserve(obj_materials.size());
        for (const Obj<3>::Material& m : obj_materials)
        {
                materials->emplace_back(m);
        }
}

//

class DrawObject final
{
        opengl::VertexArray m_vertex_array;
        std::unique_ptr<opengl::ArrayBuffer> m_vertex_buffer;
        std::unique_ptr<opengl::StorageBuffer> m_storage_buffer;
        std::vector<opengl::TextureRGBA32F> m_textures;
        unsigned m_vertices_count;

        const mat4 m_model_matrix;
        const DrawType m_draw_type;

public:
        DrawObject(const Obj<3>& obj, double size, const vec3& position);

        void bind_vertices() const;
        void bind_buffer() const;

        const mat4& model_matrix() const;
        unsigned vertices_count() const;
        DrawType draw_type() const;
};

DrawObject::DrawObject(const Obj<3>& obj, double size, const vec3& position)
        : m_model_matrix(model_vertex_matrix(obj, size, position)), m_draw_type(draw_type_of_obj(obj))
{
        if (m_draw_type == DrawType::Triangles)
        {
                std::vector<FaceVertex> vertices;
                load_face_vertices(obj, &vertices);
                m_vertices_count = vertices.size();

                m_vertex_buffer = std::make_unique<opengl::ArrayBuffer>(vertices);

                m_vertex_array.attrib_pointer(0, 3, GL_FLOAT, *m_vertex_buffer, offsetof(FaceVertex, v), sizeof(FaceVertex),
                                              true);
                m_vertex_array.attrib_pointer(1, 3, GL_FLOAT, *m_vertex_buffer, offsetof(FaceVertex, n), sizeof(FaceVertex),
                                              true);
                m_vertex_array.attrib_pointer(2, 2, GL_FLOAT, *m_vertex_buffer, offsetof(FaceVertex, t), sizeof(FaceVertex),
                                              true);
                m_vertex_array.attrib_i_pointer(3, 1, GL_INT, *m_vertex_buffer, offsetof(FaceVertex, index), sizeof(FaceVertex),
                                                true);
                m_vertex_array.attrib_i_pointer(4, 1, GL_UNSIGNED_BYTE, *m_vertex_buffer, offsetof(FaceVertex, property),
                                                sizeof(FaceVertex), true);

                //

                for (const Obj<3>::Image& image : obj.images())
                {
                        m_textures.emplace_back(image.size[0], image.size[1], image.srgba_pixels);
                }

                //

                std::vector<Material> materials;
                load_materials(obj, &materials);
                for (Material& m : materials)
                {
                        if (m.map_Ka >= 0)
                        {
                                m.map_Ka_handle = m_textures[m.map_Ka].texture().texture_resident_handle();
                        }
                        if (m.map_Kd >= 0)
                        {
                                m.map_Kd_handle = m_textures[m.map_Kd].texture().texture_resident_handle();
                        }
                        if (m.map_Ks >= 0)
                        {
                                m.map_Ks_handle = m_textures[m.map_Ks].texture().texture_resident_handle();
                        }
                }

                m_storage_buffer = std::make_unique<opengl::StorageBuffer>(materials);
        }
        else
        {
                std::vector<PointVertex> vertices;

                if (m_draw_type == DrawType::Points)
                {
                        load_point_vertices(obj, &vertices);
                }
                else
                {
                        load_line_vertices(obj, &vertices);
                }

                m_vertices_count = vertices.size();
                m_vertex_buffer = std::make_unique<opengl::ArrayBuffer>(vertices);
                m_vertex_array.attrib_pointer(0, 3, GL_FLOAT, *m_vertex_buffer, offsetof(PointVertex, v), sizeof(PointVertex),
                                              true);
        }
}
void DrawObject::bind_vertices() const
{
        m_vertex_array.bind();
}
void DrawObject::bind_buffer() const
{
        m_storage_buffer->bind(BUFFER_BINDING);
}
const mat4& DrawObject::model_matrix() const
{
        return m_model_matrix;
}
unsigned DrawObject::vertices_count() const
{
        return m_vertices_count;
}
DrawType DrawObject::draw_type() const
{
        return m_draw_type;
}

std::string color_space_message(bool framebuffer_is_srgb, bool colorbuffer_is_srgb)
{
        std::string msg;
        msg += "OpenGL renderer framebuffer color space is ";
        msg += framebuffer_is_srgb ? "sRGB" : "linear";
        msg += '\n';
        msg += "OpenGL renderer colorbuffer color space is ";
        msg += colorbuffer_is_srgb ? "sRGB" : "linear";
        return msg;
}

namespace shaders = opengl_renderer_shaders;

class Renderer final : public OpenGLRenderer
{
        static constexpr mat4 SCALE = scale<double>(0.5, 0.5, 0.5);
        static constexpr mat4 TRANSLATE = translate<double>(1, 1, 1);
        const mat4 SCALE_BIAS_MATRIX = SCALE * TRANSLATE;

        opengl::GraphicsProgram main_program, shadow_program, points_program;

        std::unique_ptr<opengl::ShadowBuffer> m_shadow_buffer;
        std::unique_ptr<opengl::ColorBuffer> m_color_buffer;
        std::unique_ptr<opengl::TextureR32I> m_objects;

        mat4 m_shadow_matrix;
        mat4 m_scale_bias_shadow_matrix;
        mat4 m_main_matrix;

        bool m_show_shadow = false;

        int m_width = -1;
        int m_height = -1;
        int m_shadow_width = -1;
        int m_shadow_height = -1;

        const int m_max_texture_size = opengl::max_texture_size();

        double m_shadow_zoom = 1;

        RendererObjectStorage<DrawObject> m_storage;

        bool m_framebuffer_srgb;
        bool m_colorbuffer_srgb;

        shaders::PointsMemory m_points_memory;
        shaders::ShadowMemory m_shadow_memory;
        shaders::TrianglesMemory m_triangles_memory;

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

        void set_matrices(const mat4& shadow_matrix, const mat4& main_matrix) override
        {
                m_shadow_matrix = shadow_matrix;
                m_scale_bias_shadow_matrix = SCALE_BIAS_MATRIX * shadow_matrix;
                m_main_matrix = main_matrix;
        }

        void set_light_direction(vec3 dir) override
        {
                m_triangles_memory.set_direction_to_light(-dir);
        }
        void set_camera_direction(vec3 dir) override
        {
                m_triangles_memory.set_direction_to_camera(-dir);
        }

        bool draw(bool draw_to_color_buffer) override
        {
                const DrawObject* draw_object = m_storage.object();

                m_objects->clear_tex_image(0);

                if (!draw_object)
                {
                        if (draw_to_color_buffer)
                        {
                                m_color_buffer->bind_buffer();
                        }
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                        if (draw_to_color_buffer)
                        {
                                m_color_buffer->unbind_buffer();
                        }
                        return false;
                }

                opengl::GLEnableAndRestore<GL_DEPTH_TEST> enable_depth_test;

                draw_object->bind_vertices();

                const DrawObject* scale_object = m_storage.scale_object();

                if (m_show_shadow && draw_object->draw_type() == DrawType::Triangles)
                {
                        m_triangles_memory.set_shadow_matrix(m_scale_bias_shadow_matrix * scale_object->model_matrix());
                        m_shadow_memory.set_matrix(m_shadow_matrix * scale_object->model_matrix());

                        m_shadow_buffer->bind_buffer();
                        glViewport(0, 0, m_shadow_width, m_shadow_height);
                        glClearDepthf(1.0f);
                        glClear(GL_DEPTH_BUFFER_BIT);

                        // depth-fighting
                        opengl::GLEnableAndRestore<GL_POLYGON_OFFSET_FILL> enable_polygon_offset_fill;
                        glPolygonOffset(2.0f, 2.0f); // glPolygonOffset(4.0f, 4.0f);

                        m_shadow_memory.bind();

                        shadow_program.draw_arrays(GL_TRIANGLES, 0, draw_object->vertices_count());

                        m_shadow_buffer->unbind_buffer();
                }

                glViewport(0, 0, m_width, m_height);

                if (draw_to_color_buffer)
                {
                        m_color_buffer->bind_buffer();
                }

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                switch (draw_object->draw_type())
                {
                case DrawType::Triangles:
                        m_triangles_memory.set_matrix(m_main_matrix * scale_object->model_matrix());
                        draw_object->bind_buffer();
                        m_triangles_memory.bind();
                        main_program.draw_arrays(GL_TRIANGLES, 0, draw_object->vertices_count());
                        break;
                case DrawType::Points:
                        m_points_memory.set_matrix(m_main_matrix * scale_object->model_matrix());
                        m_points_memory.bind();
                        points_program.draw_arrays(GL_POINTS, 0, draw_object->vertices_count());
                        break;
                case DrawType::Lines:
                        m_points_memory.set_matrix(m_main_matrix * scale_object->model_matrix());
                        m_points_memory.bind();
                        points_program.draw_arrays(GL_LINES, 0, draw_object->vertices_count());
                        break;
                }

                if (draw_to_color_buffer)
                {
                        m_color_buffer->unbind_buffer();
                }

                return true;
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

                m_shadow_buffer = std::make_unique<opengl::ShadowBuffer>(m_shadow_width, m_shadow_height);
                main_program.set_uniform_handle("shadow_tex",
                                                m_shadow_buffer->depth_texture().texture().texture_resident_handle());
        }

        void set_shadow_zoom(double zoom) override
        {
                m_shadow_zoom = zoom;

                set_shadow_size();
        }

        void set_size(int width, int height) override
        {
                m_width = width;
                m_height = height;

                m_color_buffer = std::make_unique<opengl::ColorBuffer>(width, height);
                m_objects = std::make_unique<opengl::TextureR32I>(width, height);

                main_program.set_uniform_handle("object_img", m_objects->image_resident_handle_write_only());
                points_program.set_uniform_handle("object_img", m_objects->image_resident_handle_write_only());

                set_shadow_size();
        }

        const opengl::TextureRGBA32F& color_buffer() const override
        {
                ASSERT(m_color_buffer);
                return m_color_buffer->color_texture();
        }
        const opengl::TextureR32I& objects() const override
        {
                ASSERT(m_objects);
                return *m_objects;
        }

        bool frame_buffer_is_srgb() override
        {
                return m_framebuffer_srgb;
        }

        bool color_buffer_is_srgb() override
        {
                return m_colorbuffer_srgb;
        }

        void object_add(const Obj<3>* obj, double size, const vec3& position, int id, int scale_id) override
        {
                m_storage.add_object(std::make_unique<DrawObject>(*obj, size, position), id, scale_id);
        }
        void object_delete(int id) override
        {
                m_storage.delete_object(id);
        }
        void object_show(int id) override
        {
                m_storage.show_object(id);
        }
        void object_delete_all() override
        {
                m_storage.delete_all();
        }

public:
        Renderer()
                : main_program(opengl::VertexShader(triangles_vert), opengl::GeometryShader(triangles_geom),
                               opengl::FragmentShader(triangles_frag)),
                  shadow_program(opengl::VertexShader(shadow_vert), opengl::FragmentShader(shadow_frag)),
                  points_program(opengl::VertexShader(points_vert), opengl::FragmentShader(points_frag))
        {
                glDisable(GL_CULL_FACE);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glEnable(GL_FRAMEBUFFER_SRGB);

                m_framebuffer_srgb = opengl::current_buffer_is_srgb();
                {
                        opengl::ColorBuffer color_buffer(1, 1);
                        color_buffer.bind_buffer();
                        m_colorbuffer_srgb = opengl::current_buffer_is_srgb();
                        color_buffer.unbind_buffer();
                }
                LOG(color_space_message(m_framebuffer_srgb, m_colorbuffer_srgb));
        }
};
}

mat4 OpenGLRenderer::ortho(double left, double right, double bottom, double top, double near, double far)
{
        return ortho_opengl<double>(left, right, bottom, top, near, far);
}

std::unique_ptr<OpenGLRenderer> create_opengl_renderer()
{
        return std::make_unique<Renderer>();
}
