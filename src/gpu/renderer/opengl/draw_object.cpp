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

        FaceVertex(vec3f v_, vec3f n_, vec2f t_) : v(v_), n(n_), t(t_)
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

std::unique_ptr<opengl::ArrayBuffer> load_face_vertices(const Obj<3>& obj, const std::vector<int>& sorted_face_indices)
{
        ASSERT(sorted_face_indices.size() == obj.facets().size());

        const std::vector<Obj<3>::Facet>& obj_faces = obj.facets();
        const std::vector<vec3f>& obj_vertices = obj.vertices();
        const std::vector<vec3f>& obj_normals = obj.normals();
        const std::vector<vec2f>& obj_texcoords = obj.texcoords();

        std::vector<FaceVertex> vertices;
        vertices.reserve(3 * obj.facets().size());

        vec3f v0, v1, v2;
        vec3f n0, n1, n2;
        vec2f t0, t1, t2;

        for (int face_index : sorted_face_indices)
        {
                const Obj<3>::Facet& f = obj_faces[face_index];

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

                vertices.emplace_back(v0, n0, t0);
                vertices.emplace_back(v1, n1, t1);
                vertices.emplace_back(v2, n2, t2);
        }

        return std::make_unique<opengl::ArrayBuffer>(vertices);
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

std::vector<opengl::TextureRGBA32F> load_textures(const Obj<3>& obj)
{
        std::vector<opengl::TextureRGBA32F> textures;

        for (const Obj<3>::Image& image : obj.images())
        {
                textures.emplace_back(image.size[0], image.size[1], image.srgba_pixels);
        }

        return textures;
}

std::unique_ptr<RendererMaterialMemory> load_materials(const Obj<3>& obj, const std::vector<opengl::TextureRGBA32F>& textures)
{
        std::vector<RendererMaterialMemory::Material> materials;
        materials.reserve(obj.materials().size() + 1);

        for (const Obj<3>::Material& material : obj.materials())
        {
                RendererMaterialMemory::Material m;

                m.Ka = material.Ka.to_rgb_vector<float>();
                m.Kd = material.Kd.to_rgb_vector<float>();
                m.Ks = material.Ks.to_rgb_vector<float>();
                m.Ns = material.Ns;
                m.use_texture_Ka = material.map_Ka >= 0 ? 1 : 0;
                m.use_texture_Kd = material.map_Kd >= 0 ? 1 : 0;
                m.use_texture_Ks = material.map_Ks >= 0 ? 1 : 0;
                m.use_material = 1;

                if (material.map_Ka >= 0)
                {
                        m.texture_Ka = textures[material.map_Ka].texture().texture_resident_handle();
                }
                if (material.map_Kd >= 0)
                {
                        m.texture_Kd = textures[material.map_Kd].texture().texture_resident_handle();
                }
                if (material.map_Ks >= 0)
                {
                        m.texture_Ks = textures[material.map_Ks].texture().texture_resident_handle();
                }

                materials.push_back(m);
        }

        RendererMaterialMemory::Material m;
        m.use_material = 0;
        materials.push_back(m);

        return std::make_unique<RendererMaterialMemory>(materials);
}
}

void DrawObject::load_triangles(const Obj<3>& obj)
{
        ASSERT(m_draw_type == DrawType::Triangles);

        std::vector<int> sorted_face_indices;
        std::vector<int> material_face_offset;
        std::vector<int> material_face_count;
        sort_facets_by_material(obj, sorted_face_indices, material_face_offset, material_face_count);

        m_vertex_buffer = load_face_vertices(obj, sorted_face_indices);

        m_vertex_array.attrib(0, 3, GL_FLOAT, *m_vertex_buffer, offsetof(FaceVertex, v), sizeof(FaceVertex));
        m_vertex_array.attrib(1, 3, GL_FLOAT, *m_vertex_buffer, offsetof(FaceVertex, n), sizeof(FaceVertex));
        m_vertex_array.attrib(2, 2, GL_FLOAT, *m_vertex_buffer, offsetof(FaceVertex, t), sizeof(FaceVertex));

        m_textures = load_textures(obj);

        m_shader_memory = load_materials(obj, m_textures);

        m_vertex_count = 3 * obj.facets().size();

        ASSERT(material_face_offset.size() == material_face_count.size());
        ASSERT(material_face_offset.size() == m_shader_memory->material_count());

        for (unsigned i = 0; i < m_shader_memory->material_count(); ++i)
        {
                if (material_face_count[i] > 0)
                {
                        m_materials.emplace_back(i, 3 * material_face_offset[i], 3 * material_face_count[i]);
                }
        }
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

        m_vertex_count = vertices.size();
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

bool DrawObject::has_shadow() const
{
        return m_draw_type == DrawType::Triangles;
}

const mat4& DrawObject::model_matrix() const
{
        return m_model_matrix;
}

void DrawObject::draw(const DrawInfo& info) const
{
        ASSERT(info.triangles_program && info.triangles_memory);
        ASSERT(info.points_program && info.points_memory);
        ASSERT(info.lines_program && info.lines_memory);

        m_vertex_array.bind();

        switch (m_draw_type)
        {
        case DrawType::Triangles:
                info.triangles_memory->bind();
                for (const Material& material : m_materials)
                {
                        ASSERT(material.vertex_count > 0);
                        m_shader_memory->bind(material.material_index);
                        info.triangles_program->draw_arrays(GL_TRIANGLES, material.vertex_offset, material.vertex_count);
                }
                break;
        case DrawType::Points:
                info.points_memory->bind();
                info.points_program->draw_arrays(GL_POINTS, 0, m_vertex_count);
                break;
        case DrawType::Lines:
                info.lines_memory->bind();
                info.lines_program->draw_arrays(GL_LINES, 0, m_vertex_count);
                break;
        }
}

void DrawObject::shadow(const ShadowInfo& info) const
{
        ASSERT(info.triangles_memory && info.triangles_program);

        m_vertex_array.bind();

        info.triangles_memory->bind();
        info.triangles_program->draw_arrays(GL_TRIANGLES, 0, m_vertex_count);
}
}
