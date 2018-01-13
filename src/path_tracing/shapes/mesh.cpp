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

#include "mesh.h"

#include "simplex_wrapper.h"

#include "com/log.h"
#include "com/mat_alg.h"
#include "com/time.h"
#include "com/vec.h"
#include "path_tracing/space/ray_intersection.h"

#include <algorithm>
#include <utility>

constexpr int OCTREE_MAX_DEPTH = 10;
constexpr int OCTREE_MIN_OBJECTS = 10;

void Mesh::create_mesh_object(const IObj* obj, const mat4& vertex_matrix, unsigned thread_count, ProgressRatio* progress)
{
        if (obj->get_vertices().size() == 0)
        {
                error("No vertices found in obj");
        }

        if (obj->get_faces().size() == 0)
        {
                error("No faces found in obj");
        }

        m_vertices = to_vector<double>(obj->get_vertices());
        m_vertices.shrink_to_fit();
        std::transform(m_vertices.begin(), m_vertices.end(), m_vertices.begin(), MatrixMulVector<double>(vertex_matrix));

        m_normals = to_vector<double>(obj->get_normals());
        m_normals.shrink_to_fit();

        m_texcoords = to_vector<double>(obj->get_texcoords());
        m_texcoords.shrink_to_fit();

        m_triangles.reserve(obj->get_faces().size());
        for (const IObj::face3& face : obj->get_faces())
        {
                int v0 = face.vertices[0].v;
                int v1 = face.vertices[1].v;
                int v2 = face.vertices[2].v;

                int vn0 = face.vertices[0].vn;
                int vn1 = face.vertices[1].vn;
                int vn2 = face.vertices[2].vn;

                int vt0 = face.vertices[0].vt;
                int vt1 = face.vertices[1].vt;
                int vt2 = face.vertices[2].vt;

                m_triangles.emplace_back(m_vertices.data(), m_normals.data(), m_texcoords.data(), v0, v1, v2, face.has_vn, vn0,
                                         vn1, vn2, face.has_vt, vt0, vt1, vt2, face.material);
        }

        m_materials.reserve(obj->get_materials().size());
        for (const IObj::material& m : obj->get_materials())
        {
                m_materials.emplace_back(to_vector<double>(m.Kd), to_vector<double>(m.Ks), m.Ns, m.map_Kd, m.map_Ks);
        }

        m_images.reserve(obj->get_images().size());
        for (const IObj::image& image : obj->get_images())
        {
                m_images.emplace_back(image.dimensions[0], image.dimensions[1], image.srgba_pixels);
        }

        progress->set_text("Octree: %v of %m");

        std::vector<SimplexWrapperForShapeIntersection<TableTriangle>> simplex_wrappers;
        simplex_wrappers.reserve(m_triangles.size());
        for (const TableTriangle& t : m_triangles)
        {
                simplex_wrappers.emplace_back(t);
        }

        // Указатель на объект октадерева
        auto lambda_triangle = [w = std::as_const(simplex_wrappers)](int triangle_index)
        {
                return &(w[triangle_index]);
        };

        m_octree.decompose(m_triangles.size(), lambda_triangle, thread_count, progress);
}

Mesh::Mesh(const IObj* obj, const mat4& vertex_matrix, unsigned thread_count, ProgressRatio* progress)
        : m_octree(OCTREE_MAX_DEPTH, OCTREE_MIN_OBJECTS)
{
        double start_time = get_time_seconds();

        create_mesh_object(obj, vertex_matrix, thread_count, progress);

        LOG("Mesh object created, " + to_string_fixed(get_time_seconds() - start_time, 5) + " s");
}

bool Mesh::intersect_approximate(const ray3& r, double* t) const
{
        return m_octree.intersect_root(r, t);
}

bool Mesh::intersect_precise(const ray3& ray, double approximate_t, double* t, const GeometricObject** geometric_object) const
{
        const TableTriangle* triangle = nullptr;

        if (m_octree.trace_ray(ray, approximate_t,
                               // Пересечение луча с набором треугольников ячейки октадерева
                               [&](const std::vector<int>& triangle_indices, vec3* point) -> bool {
                                       if (ray_intersection(m_triangles, triangle_indices, ray, t, &triangle))
                                       {
                                               *point = ray.point(*t);
                                               return true;
                                       }
                                       return false;
                               }))
        {
                *geometric_object = triangle;
                return true;
        }
        else
        {
                return false;
        }
}

vec3 Mesh::get_geometric_normal(const GeometricObject* geometric_object) const
{
        return static_cast<const TableTriangle*>(geometric_object)->geometric_normal();
}

vec3 Mesh::get_shading_normal(const vec3& p, const GeometricObject* geometric_object) const
{
        return static_cast<const TableTriangle*>(geometric_object)->shading_normal(p);
}

std::optional<vec3> Mesh::get_color(const vec3& p, const GeometricObject* geometric_object) const
{
        const TableTriangle* triangle = static_cast<const TableTriangle*>(geometric_object);

        if (triangle->get_material() >= 0)
        {
                const Material& m = m_materials[triangle->get_material()];

                if (triangle->has_texcoord() && m.map_Kd >= 0)
                {
                        return m_images[m.map_Kd].get_texture(triangle->texcoord(p));
                }
                else
                {
                        return m.Kd;
                }
        }
        else
        {
                return std::nullopt;
        }
}
