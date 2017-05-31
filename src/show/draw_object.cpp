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

#include "gl/gl_objects.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>

namespace
{
// Структуры данных для передачи данных в шейдеры

struct Vertex final
{
        glm::vec3 v; // Координаты вершины в пространстве.
        glm::vec3 n; // Нормаль вершины.
        glm::vec2 t; // Координаты вершины в текстуре.
        GLint index; // номер материала.
        // Бит 0: Заданы ли текстурные координаты. Если не заданы текстурные координаты, то использовать цвет материала.
        // Бит 1: Задана ли нормаль. Если не задана нормаль, то использовать одинаковую нормаль для всего треугольника.
        GLubyte property;

        Vertex(glm::vec3 v_, glm::vec3 n_, glm::vec2 t_, GLint index_, bool has_tex_coord_, bool has_normal_)
                : v(v_), n(n_), t(t_), index(index_)
        {
                property = 0;
                property |= (has_tex_coord_ ? 0b1 : 0);
                property |= (has_normal_ ? 0b10 : 0);
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

void load_vertices(const IObj& obj, std::vector<Vertex>* vertices)
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

public:
        DrawObject(const std::shared_ptr<IObj>& obj_ptr, const ColorSpaceConverter& color_converter);

        const glm::mat4& get_model_matrix() const override
        {
                return m_model_matrix;
        }
        void bind_vertex_array() const override
        {
                m_vertex_array.bind();
        }
        void bind_storage_buffer(unsigned binding_point) const override
        {
                m_storage_buffer.bind(binding_point);
        }
        unsigned get_vertices_count() const override
        {
                return m_vertices_count;
        }
};

DrawObject::DrawObject(const std::shared_ptr<IObj>& obj_ptr, const ColorSpaceConverter& color_converter)
{
        std::vector<Vertex> vertices;
        load_vertices(*obj_ptr, &vertices);
        m_vertices_count = vertices.size();
        m_vertex_buffer.load_static_draw(vertices);

        m_vertex_array.attrib_pointer(0, 3, GL_FLOAT, m_vertex_buffer, offsetof(Vertex, v), sizeof(Vertex), true);
        m_vertex_array.attrib_pointer(1, 3, GL_FLOAT, m_vertex_buffer, offsetof(Vertex, n), sizeof(Vertex), true);
        m_vertex_array.attrib_pointer(2, 2, GL_FLOAT, m_vertex_buffer, offsetof(Vertex, t), sizeof(Vertex), true);
        m_vertex_array.attrib_i_pointer(3, 1, GL_INT, m_vertex_buffer, offsetof(Vertex, index), sizeof(Vertex), true);
        m_vertex_array.attrib_i_pointer(4, 1, GL_UNSIGNED_BYTE, m_vertex_buffer, offsetof(Vertex, property), sizeof(Vertex),
                                        true);

        //

        const std::vector<sf::Image>& images = obj_ptr->get_images();

        for (size_t i = 0; i < images.size(); ++i)
        {
                m_textures.emplace_back(images[i]);

                // преобразование sRGB в RGB
                color_converter.convert(m_textures[m_textures.size() - 1].get_texture());
        }

        //

        std::vector<Material> materials;
        load_materials(*obj_ptr, &materials);
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

        //

        glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(2.0f / obj_ptr->get_length()));
        glm::mat4 translate = glm::translate(glm::mat4(1), -obj_ptr->get_center());
        m_model_matrix = scale * translate;
}
}

std::unique_ptr<IDrawObject> create_draw_object(const std::shared_ptr<IObj>& obj_ptr, const ColorSpaceConverter& color_converter)
{
        return std::make_unique<DrawObject>(obj_ptr, color_converter);
}
