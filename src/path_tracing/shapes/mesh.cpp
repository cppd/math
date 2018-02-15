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

void Mesh::create_mesh_object(const Obj<3>* obj, const mat4& vertex_matrix, unsigned thread_count, ProgressRatio* progress)
{
        if (obj->vertices().size() == 0)
        {
                error("No vertices found in obj");
        }

        if (obj->facets().size() == 0)
        {
                error("No facets found in obj");
        }

        m_vertices = to_vector<double>(obj->vertices());
        m_vertices.shrink_to_fit();
        std::transform(m_vertices.begin(), m_vertices.end(), m_vertices.begin(), MatrixMulVector<double>(vertex_matrix));

        m_normals = to_vector<double>(obj->normals());
        m_normals.shrink_to_fit();

        m_texcoords = to_vector<double>(obj->texcoords());
        m_texcoords.shrink_to_fit();

        m_triangles.reserve(obj->facets().size());
        for (const Obj<3>::Facet& facet : obj->facets())
        {
                m_triangles.emplace_back(m_vertices, m_normals, m_texcoords, facet.vertices, facet.has_normal, facet.normals,
                                         facet.has_texcoord, facet.texcoords, facet.material);
        }

        m_materials.reserve(obj->materials().size());
        for (const Obj<3>::Material& m : obj->materials())
        {
                m_materials.emplace_back(m.Kd, m.Ks, m.Ns, m.map_Kd, m.map_Ks);
        }

        m_images.reserve(obj->images().size());
        for (const Obj<3>::Image& image : obj->images())
        {
                m_images.emplace_back(image.size, image.srgba_pixels);
        }

        progress->set_text("Octree: %v of %m");

        std::vector<SimplexWrapperForShapeIntersection<Simplex>> simplex_wrappers;
        simplex_wrappers.reserve(m_triangles.size());
        for (const Simplex& t : m_triangles)
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

Mesh::Mesh(const Obj<3>* obj, const mat4& vertex_matrix, unsigned thread_count, ProgressRatio* progress)
        : m_octree(OCTREE_MAX_DEPTH, OCTREE_MIN_OBJECTS)
{
        double start_time = time_in_seconds();

        create_mesh_object(obj, vertex_matrix, thread_count, progress);

        LOG("Mesh object created, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
}

bool Mesh::intersect_approximate(const ray3& r, double* t) const
{
        return m_octree.intersect_root(r, t);
}

bool Mesh::intersect_precise(const ray3& ray, double approximate_t, double* t, const void** intersection_data) const
{
        const Simplex* triangle = nullptr;

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
                *intersection_data = triangle;
                return true;
        }
        else
        {
                return false;
        }
}

vec3 Mesh::get_geometric_normal(const void* intersection_data) const
{
        return static_cast<const Simplex*>(intersection_data)->geometric_normal();
}

vec3 Mesh::get_shading_normal(const vec3& p, const void* intersection_data) const
{
        return static_cast<const Simplex*>(intersection_data)->shading_normal(p);
}

std::optional<Color> Mesh::get_color(const vec3& p, const void* intersection_data) const
{
        const Simplex* triangle = static_cast<const Simplex*>(intersection_data);

        if (triangle->material() >= 0)
        {
                const Material& m = m_materials[triangle->material()];

                if (triangle->has_texcoord() && m.map_Kd >= 0)
                {
                        return m_images[m.map_Kd].texture(triangle->texcoord(p));
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
