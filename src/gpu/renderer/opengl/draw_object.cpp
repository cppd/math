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

#include "draw_object.h"

#include "com/error.h"
#include "gpu/com/glsl.h"
#include "obj/alg/alg.h"

namespace gpu_opengl
{
// Число используется в шейдере для определения наличия текстурных координат
constexpr vec2f NO_TEXTURE_COORDINATES = vec2f(-1e10);

namespace
{
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

        FaceVertex(vec3f v_, vec3f n_, vec2f t_, GLint index_) : v(v_), n(n_), t(t_), index(index_)
        {
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
        alignas(GLSL_VEC3_ALIGN) vec3f Ka;
        alignas(GLSL_VEC3_ALIGN) vec3f Kd;
        alignas(GLSL_VEC3_ALIGN) vec3f Ks;

        GLuint64 map_Ka_handle, map_Kd_handle, map_Ks_handle;

        GLfloat Ns;

        GLuint use_map_Ka, use_map_Kd, use_map_Ks;

        explicit Material(const Obj<3>::Material& m)
                : Ka(m.Ka.to_rgb_vector<float>()),
                  Kd(m.Kd.to_rgb_vector<float>()),
                  Ks(m.Ks.to_rgb_vector<float>()),
                  Ns(m.Ns),
                  use_map_Ka(m.map_Ka >= 0 ? 1 : 0),
                  use_map_Kd(m.map_Kd >= 0 ? 1 : 0),
                  use_map_Ks(m.map_Ks >= 0 ? 1 : 0)
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
                        vec3f geometric_normal = normalize(cross(v1 - v0, v2 - v0));
                        if (!is_finite(geometric_normal))
                        {
                                error("Face unit orthogonal vector is not finite for the face with vertices (" + to_string(v0) +
                                      ", " + to_string(v1) + ", " + to_string(v2) + ")");
                        }

                        n0 = n1 = n2 = geometric_normal;
                }

                if (f.has_texcoord)
                {
                        t0 = obj_texcoords[f.texcoords[0]];
                        t1 = obj_texcoords[f.texcoords[1]];
                        t2 = obj_texcoords[f.texcoords[2]];
                }
                else
                {
                        t0 = t1 = t2 = NO_TEXTURE_COORDINATES;
                }

                vertices->emplace_back(v0, n0, t0, f.material);
                vertices->emplace_back(v1, n1, t1, f.material);
                vertices->emplace_back(v2, n2, t2, f.material);
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

void load_materials(const Obj<3>& obj, const std::vector<opengl::TextureRGBA32F>& textures, std::vector<Material>* materials)
{
        const std::vector<Obj<3>::Material>& obj_materials = obj.materials();

        materials->clear();
        materials->shrink_to_fit();
        materials->reserve(obj_materials.size());
        for (const Obj<3>::Material& m : obj_materials)
        {
                materials->emplace_back(m);
                if (m.map_Ka >= 0)
                {
                        materials->back().map_Ka_handle = textures[m.map_Ka].texture().texture_resident_handle();
                }
                if (m.map_Kd >= 0)
                {
                        materials->back().map_Kd_handle = textures[m.map_Kd].texture().texture_resident_handle();
                }
                if (m.map_Ks >= 0)
                {
                        materials->back().map_Ks_handle = textures[m.map_Ks].texture().texture_resident_handle();
                }
        }
}
}

void DrawObject::load_triangles(const Obj<3>& obj)
{
        ASSERT(m_draw_type == DrawType::Triangles);

        std::vector<FaceVertex> vertices;
        load_face_vertices(obj, &vertices);
        m_vertices_count = vertices.size();

        m_vertex_buffer = std::make_unique<opengl::ArrayBuffer>(vertices);

        m_vertex_array.attrib(0, 3, GL_FLOAT, *m_vertex_buffer, offsetof(FaceVertex, v), sizeof(FaceVertex));
        m_vertex_array.attrib(1, 3, GL_FLOAT, *m_vertex_buffer, offsetof(FaceVertex, n), sizeof(FaceVertex));
        m_vertex_array.attrib(2, 2, GL_FLOAT, *m_vertex_buffer, offsetof(FaceVertex, t), sizeof(FaceVertex));
        m_vertex_array.attrib_i(3, 1, GL_INT, *m_vertex_buffer, offsetof(FaceVertex, index), sizeof(FaceVertex));

        //

        for (const Obj<3>::Image& image : obj.images())
        {
                m_textures.emplace_back(image.size[0], image.size[1], image.srgba_pixels);
        }

        //

        std::vector<Material> materials;
        load_materials(obj, m_textures, &materials);

        m_storage_buffer = std::make_unique<opengl::StorageBuffer>(materials);
}

void DrawObject::load_points_lines(const Obj<3>& obj)
{
        ASSERT(m_draw_type == DrawType::Points || m_draw_type == DrawType::Lines);

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
        m_vertex_array.attrib(0, 3, GL_FLOAT, *m_vertex_buffer, offsetof(PointVertex, v), sizeof(PointVertex));
}

DrawObject::DrawObject(const Obj<3>& obj, double size, const vec3& position)
        : m_model_matrix(model_vertex_matrix(obj, size, position)), m_draw_type(draw_type_of_obj(obj))
{
        if (m_draw_type == DrawType::Triangles)
        {
                load_triangles(obj);
        }
        else
        {
                load_points_lines(obj);
        }
}

void DrawObject::bind_vertices() const
{
        m_vertex_array.bind();
}

const opengl::StorageBuffer* DrawObject::materials() const
{
        return m_storage_buffer.get();
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
}
