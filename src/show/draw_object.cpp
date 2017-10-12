/*
Copyright (C) 2017 Topological Manifold

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

#include "draw_object.h"

#include "com/log.h"
#include "com/print.h"
#include "graphics/query.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec2.hpp>
#include <vector>

// clang-format off
constexpr const char triangles_vert[]
{
#include "draw_triangles.vert.str"
};
constexpr const char triangles_geom[]
{
#include "draw_triangles.geom.str"
};
constexpr const char triangles_frag[]
{
#include "draw_triangles.frag.str"
};
constexpr const char shadow_vert[]
{
#include "draw_shadow.vert.str"
};
constexpr const char shadow_frag[]
{
#include "draw_shadow.frag.str"
};
constexpr const char points_vert[]
{
#include "draw_points.vert.str"
};
constexpr const char points_frag[]
{
#include "draw_points.frag.str"
};
// clang-format on

namespace
{
// Структуры данных для передачи данных в шейдеры

struct FaceVertex final
{
        glm::vec3 v; // Координаты вершины в пространстве.
        glm::vec3 n; // Нормаль вершины.
        glm::vec2 t; // Координаты вершины в текстуре.
        GLint index; // номер материала.
        // Бит 0: Заданы ли текстурные координаты. Если не заданы текстурные координаты, то использовать цвет материала.
        // Бит 1: Задана ли нормаль. Если не задана нормаль, то использовать одинаковую нормаль для всего треугольника.
        GLubyte property;

        FaceVertex(glm::vec3 v_, glm::vec3 n_, glm::vec2 t_, GLint index_, bool has_tex_coord_, bool has_normal_)
                : v(v_), n(n_), t(t_), index(index_)
        {
                property = 0;
                property |= (has_tex_coord_ ? 0b1 : 0);
                property |= (has_normal_ ? 0b10 : 0);
        }
};

struct PointVertex final
{
        glm::vec3 v; // Координаты вершины в пространстве.

        PointVertex(glm::vec3 v_) : v(v_)
        {
        }
};

// shader storage
struct Material final
{
        // layout std430: alignof vec3 = 4 * sizeof GLfloat
        alignas(sizeof(glm::vec4)) glm::vec3 Ka, Kd, Ks;

        GLuint64 map_Ka_handle, map_Kd_handle, map_Ks_handle;

        GLfloat Ns;

        // если нет текстуры, то -1
        GLint map_Ka, map_Kd, map_Ks;

        explicit Material(const IObj::material& m)
                : Ka(m.Ka), Kd(m.Kd), Ks(m.Ks), Ns(m.Ns), map_Ka(m.map_Ka), map_Kd(m.map_Kd), map_Ks(m.map_Ks)
        {
        }
};

void load_face_vertices(const IObj& obj, std::vector<FaceVertex>* vertices)
{
        const std::vector<IObj::face3>& obj_faces = obj.get_faces();
        const std::vector<glm::vec3>& obj_vertices = obj.get_vertices();
        const std::vector<glm::vec2>& obj_texcoords = obj.get_texcoords();
        const std::vector<glm::vec3>& obj_normals = obj.get_normals();

        vertices->clear();
        vertices->shrink_to_fit();
        vertices->reserve(obj_faces.size() * 3);

        glm::vec3 v0, v1, v2, n0, n1, n2;
        glm::vec2 t0, t1, t2;

        for (const IObj::face3& f : obj_faces)
        {
                v0 = obj_vertices[f.vertices[0].v];
                v1 = obj_vertices[f.vertices[1].v];
                v2 = obj_vertices[f.vertices[2].v];

                if (f.has_vn)
                {
                        n0 = obj_normals[f.vertices[0].vn];
                        n1 = obj_normals[f.vertices[1].vn];
                        n2 = obj_normals[f.vertices[2].vn];
                }
                else
                {
                        n0 = n1 = n2 = glm::vec3(0.0f, 0.0f, 0.0f);
                        // можно один раз вычислять здесь, вместо геометрического шейдера
                        // n0 = n1 = n2 = glm::normalize(glm::cross(v1 - v0, v2 - v0));
                }

                if (f.has_vt)
                {
                        t0 = obj_texcoords[f.vertices[0].vt];
                        t1 = obj_texcoords[f.vertices[1].vt];
                        t2 = obj_texcoords[f.vertices[2].vt];
                }
                else
                {
                        t0 = t1 = t2 = glm::vec2(0.0f, 0.0f);
                }

                vertices->emplace_back(v0, n0, t0, f.material, f.has_vt, f.has_vn);
                vertices->emplace_back(v1, n1, t1, f.material, f.has_vt, f.has_vn);
                vertices->emplace_back(v2, n2, t2, f.material, f.has_vt, f.has_vn);
        }
}

void load_point_vertices(const IObj& obj, std::vector<PointVertex>* vertices)
{
        const std::vector<int>& obj_points = obj.get_points();
        const std::vector<glm::vec3>& obj_vertices = obj.get_vertices();

        vertices->clear();
        vertices->shrink_to_fit();
        vertices->reserve(obj_points.size());

        glm::vec3 v;
        for (int p : obj_points)
        {
                v = obj_vertices[p];
                vertices->emplace_back(v);
        }
}

void load_materials(const IObj& obj, std::vector<Material>* materials)
{
        const std::vector<IObj::material>& obj_materials = obj.get_materials();

        materials->clear();
        materials->shrink_to_fit();
        materials->reserve(obj_materials.size());
        for (const IObj::material& m : obj_materials)
        {
                materials->emplace_back(m);
        }
}

class DrawObject final : public IDrawObject
{
        VertexArray m_vertex_array;
        ArrayBuffer m_vertex_buffer;
        ShaderStorageBuffer m_storage_buffer;
        std::vector<TextureRGBA32F> m_textures;
        unsigned m_vertices_count;
        glm::mat4 m_model_matrix;
        bool m_triangles;

        const glm::mat4& get_model_matrix() const override
        {
                return m_model_matrix;
        }
        void bind() const override
        {
                m_vertex_array.bind();
                m_storage_buffer.bind(0);
        }
        unsigned get_vertices_count() const override
        {
                return m_vertices_count;
        }
        virtual bool has_triangles() const override
        {
                return m_triangles;
        }

public:
        DrawObject(const IObj* obj, const ColorSpaceConverter& color_converter, float size, glm::vec3 position);
};

DrawObject::DrawObject(const IObj* obj, const ColorSpaceConverter& color_converter, float size, glm::vec3 position)
{
        if (obj->get_faces().size() != 0 && obj->get_points().size() != 0)
        {
                error("Faces and points not supported");
        }
        if (obj->get_faces().size() == 0 && obj->get_points().size() == 0)
        {
                error("Faces or points not found");
        }

        m_triangles = obj->get_faces().size() > 0;

        if (obj->get_faces().size() > 0)
        {
                std::vector<FaceVertex> vertices;
                load_face_vertices(*obj, &vertices);
                m_vertices_count = vertices.size();

                m_vertex_buffer.load_static_draw(vertices);

                m_vertex_array.attrib_pointer(0, 3, GL_FLOAT, m_vertex_buffer, offsetof(FaceVertex, v), sizeof(FaceVertex), true);
                m_vertex_array.attrib_pointer(1, 3, GL_FLOAT, m_vertex_buffer, offsetof(FaceVertex, n), sizeof(FaceVertex), true);
                m_vertex_array.attrib_pointer(2, 2, GL_FLOAT, m_vertex_buffer, offsetof(FaceVertex, t), sizeof(FaceVertex), true);
                m_vertex_array.attrib_i_pointer(3, 1, GL_INT, m_vertex_buffer, offsetof(FaceVertex, index), sizeof(FaceVertex),
                                                true);
                m_vertex_array.attrib_i_pointer(4, 1, GL_UNSIGNED_BYTE, m_vertex_buffer, offsetof(FaceVertex, property),
                                                sizeof(FaceVertex), true);

                //

                const std::vector<sf::Image>& images = obj->get_images();

                for (size_t i = 0; i < images.size(); ++i)
                {
                        m_textures.emplace_back(images[i]);

                        // преобразование sRGB в RGB
                        color_converter.convert(m_textures[m_textures.size() - 1].get_texture());
                }

                //

                std::vector<Material> materials;
                load_materials(*obj, &materials);
                for (Material& m : materials)
                {
                        if (m.map_Ka >= 0)
                        {
                                m.map_Ka_handle = m_textures[m.map_Ka].get_texture().get_texture_resident_handle();
                        }
                        if (m.map_Kd >= 0)
                        {
                                m.map_Kd_handle = m_textures[m.map_Kd].get_texture().get_texture_resident_handle();
                        }
                        if (m.map_Ks >= 0)
                        {
                                m.map_Ks_handle = m_textures[m.map_Ks].get_texture().get_texture_resident_handle();
                        }
                }

                m_storage_buffer.load_static_draw(materials);
        }
        else
        {
                std::vector<PointVertex> vertices;
                load_point_vertices(*obj, &vertices);
                m_vertices_count = vertices.size();

                m_vertex_buffer.load_static_draw(vertices);

                m_vertex_array.attrib_pointer(0, 3, GL_FLOAT, m_vertex_buffer, offsetof(PointVertex, v), sizeof(PointVertex),
                                              true);
        }

        glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(size / obj->get_length()));
        glm::mat4 translate = glm::translate(glm::mat4(1), -obj->get_center());
        m_model_matrix = glm::translate(glm::mat4(1), position) * scale * translate;
}

class DrawProgram final : public IDrawProgram
{
        const glm::mat4 SB_SCALE = glm::scale(glm::mat4(1), glm::vec3(0.5f, 0.5f, 0.5f));
        const glm::mat4 SB_TRANSLATE = glm::translate(glm::mat4(1), glm::vec3(1, 1, 1));
        const glm::mat4 SCALE_BIAS_MATRIX = SB_SCALE * SB_TRANSLATE;

        GraphicsProgram main_program, shadow_program, points_program;

        std::unique_ptr<ShadowBuffer> m_shadow_buffer;
        std::unique_ptr<ColorBuffer> m_color_buffer;
        std::unique_ptr<TextureR32I> m_object_texture;

        glm::mat4 m_shadow_matrix;
        glm::mat4 m_scale_bias_shadow_matrix;
        glm::mat4 m_main_matrix;

        int m_width = -1;
        int m_height = -1;
        int m_shadow_width = -1;
        int m_shadow_height = -1;

        const int m_max_texture_size = get_max_texture_size();

        float m_shadow_zoom = 1;

        void set_light_a(glm::vec4 light) override
        {
                main_program.set_uniform("light_a", light);
                points_program.set_uniform("light_a", light);
        }
        void set_light_d(glm::vec4 light) override
        {
                main_program.set_uniform("light_d", light);
        }
        void set_light_s(glm::vec4 light) override
        {
                main_program.set_uniform("light_s", light);
        }
        void set_default_color(glm::vec4 color) override
        {
                main_program.set_uniform("default_color", color);
                points_program.set_uniform("default_color", color);
        }
        void set_wireframe_color(glm::vec4 color) override
        {
                main_program.set_uniform("wireframe_color", color);
        }
        void set_default_ns(float default_ns) override
        {
                main_program.set_uniform("default_ns", default_ns);
        }
        void set_show_smooth(bool show) override
        {
                main_program.set_uniform("show_smooth", show ? 1 : 0);
        }
        void set_show_wireframe(bool show) override
        {
                main_program.set_uniform("show_wireframe", show ? 1 : 0);
        }
        void set_show_shadow(bool show) override
        {
                main_program.set_uniform("show_shadow", show ? 1 : 0);
        }
        void set_show_materials(bool show) override
        {
                main_program.set_uniform("show_materials", show ? 1 : 0);
        }

        void set_matrices(const glm::mat4& shadow_matrix, const glm::mat4& main_matrix) override
        {
                m_shadow_matrix = shadow_matrix;
                m_scale_bias_shadow_matrix = SCALE_BIAS_MATRIX * shadow_matrix;
                m_main_matrix = main_matrix;
        }

        void set_light_direction(glm::vec3 dir) override
        {
                main_program.set_uniform("light_direction", dir);
        }
        void set_camera_direction(glm::vec3 dir) override
        {
                main_program.set_uniform("camera_direction", dir);
        }

        void draw(const IDrawObject* draw_object, const IDrawObject* draw_scale_object, bool shadow_active,
                  bool draw_to_buffer) override
        {
                m_object_texture->clear_tex_image(0);

                if (!draw_object)
                {
                        return;
                }

                draw_object->bind();

                const glm::mat4& model_matrix =
                        draw_scale_object ? draw_scale_object->get_model_matrix() : draw_object->get_model_matrix();

                if (shadow_active && draw_object->has_triangles())
                {
                        shadow_program.set_uniform("mvpMatrix", m_shadow_matrix * model_matrix);

                        m_shadow_buffer->bind_buffer();
                        glViewport(0, 0, m_shadow_width, m_shadow_height);
                        glClearDepthf(1.0f);
                        glClear(GL_DEPTH_BUFFER_BIT);
                        glEnable(GL_POLYGON_OFFSET_FILL); // depth-fighting
                        glPolygonOffset(2.0f, 2.0f); // glPolygonOffset(4.0f, 4.0f);

                        shadow_program.draw_arrays(GL_TRIANGLES, 0, draw_object->get_vertices_count());

                        glDisable(GL_POLYGON_OFFSET_FILL);
                        m_shadow_buffer->unbind_buffer();
                }

                glViewport(0, 0, m_width, m_height);

                if (draw_to_buffer)
                {
                        m_color_buffer->bind_buffer();
                        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                }

                if (draw_object->has_triangles())
                {
                        main_program.set_uniform("shadowMatrix", m_scale_bias_shadow_matrix * model_matrix);
                        main_program.set_uniform("mvpMatrix", m_main_matrix * model_matrix);

                        main_program.draw_arrays(GL_TRIANGLES, 0, draw_object->get_vertices_count());
                }
                else
                {
                        points_program.set_uniform("mvpMatrix", m_main_matrix * model_matrix);

                        points_program.draw_arrays(GL_POINTS, 0, draw_object->get_vertices_count());
                }

                if (draw_to_buffer)
                {
                        m_color_buffer->unbind_buffer();
                }
        }

        void free_buffers() override
        {
                m_shadow_buffer = nullptr;
                m_color_buffer = nullptr;
                m_object_texture = nullptr;
                m_width = m_height = -1;
        }

        void set_shadow_size()
        {
                if (m_width < 0 || m_height <= 0)
                {
                        return;
                }

                m_shadow_width = m_shadow_zoom * m_width;
                m_shadow_height = m_shadow_zoom * m_height;

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

                m_shadow_buffer = std::make_unique<ShadowBuffer>(m_shadow_width, m_shadow_height);
                main_program.set_uniform_handle("shadow_tex", m_shadow_buffer->get_texture().get_texture_resident_handle());
        }

        void set_shadow_zoom(float zoom) override
        {
                m_shadow_zoom = zoom;

                set_shadow_size();
        }

        void set_size(int width, int height) override
        {
                m_width = width;
                m_height = height;

                m_color_buffer = std::make_unique<ColorBuffer>(width, height);
                m_object_texture = std::make_unique<TextureR32I>(width, height);

                main_program.set_uniform_handle("object_img", m_object_texture->get_image_resident_handle_write_only());
                points_program.set_uniform_handle("object_img", m_object_texture->get_image_resident_handle_write_only());

                set_shadow_size();
        }

        const Texture2D& get_color_buffer_texture() const override
        {
                ASSERT(m_color_buffer);
                return m_color_buffer->get_texture();
        }
        const TextureR32I& get_object_texture() const override
        {
                ASSERT(m_object_texture);
                return *m_object_texture;
        }

public:
        DrawProgram()
                : main_program(VertexShader(triangles_vert), GeometryShader(triangles_geom), FragmentShader(triangles_frag)),
                  shadow_program(VertexShader(shadow_vert), FragmentShader(shadow_frag)),
                  points_program(VertexShader(points_vert), FragmentShader(points_frag))
        {
        }
};
}

std::unique_ptr<IDrawObject> create_draw_object(const IObj* obj, const ColorSpaceConverter& color_converter, float size,
                                                glm::vec3 position)
{
        return std::make_unique<DrawObject>(obj, color_converter, size, position);
}

std::unique_ptr<IDrawProgram> create_draw_program()
{
        return std::make_unique<DrawProgram>();
}
