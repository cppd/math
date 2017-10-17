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

#include "visible_mesh.h"

#include "ray_intersection.h"

#include "com/log.h"
#include "com/time.h"
#include "com/vec.h"
#include "com/vec_glm.h"
#include "path_tracing/shapes/intersection.h"

constexpr int OCTREE_MAX_DEPTH = 10;
constexpr int OCTREE_MIN_OBJECTS = 10;

void VisibleMesh::create_mesh_object(const IObj* obj, double size, const vec3& position, ProgressRatio* progress)
{
        if (obj->get_vertices().size() == 0)
        {
                error("No vertices found in obj");
        }
        if (obj->get_faces().size() == 0)
        {
                error("No faces found in obj");
        }

        m_vertices.reserve(obj->get_vertices().size());
        m_vertices = to_vector<double>(obj->get_vertices());

        m_normals.reserve(obj->get_normals().size());
        m_normals = to_vector<double>(obj->get_normals());

        m_texcoords.reserve(obj->get_texcoords().size());
        m_texcoords = to_vector<double>(obj->get_texcoords());

        vec3 center = to_vector<double>(obj->get_center());
        double scale = size / obj->get_length();
        for (vec3& v : m_vertices)
        {
                v = (v - center) * scale + position;
        }

        for (const IObj::face3& face : obj->get_faces())
        {
                m_triangles.emplace_back(m_vertices.data(), m_normals.data(), m_texcoords.data(), face.vertices[0].v,
                                         face.vertices[1].v, face.vertices[2].v, face.has_vn, face.vertices[0].vn,
                                         face.vertices[1].vn, face.vertices[2].vn, face.has_vt, face.vertices[0].vt,
                                         face.vertices[1].vt, face.vertices[2].vt, face.material);
        }

        m_materials.reserve(obj->get_materials().size());
        for (const IObj::material& m : obj->get_materials())
        {
                m_materials.emplace_back(to_vector<double>(m.Kd), to_vector<double>(m.Ks), m.Ns, m.map_Kd, m.map_Ks);
        }

        m_images.reserve(obj->get_images().size());
        for (const sf::Image& i : obj->get_images())
        {
                m_images.emplace_back(i);
        }

        progress->set_text("Octree: %v of %m");

        // clang-format off
        m_octree.decompose
                (
                        m_triangles,

                        // вершины выпуклой оболочки помещаемого в октадерево объекта
                        [](const TableTriangle& t) -> std::vector<vec3>
                        {
                                return {t.v0(), t.v1(), t.v2()};
                        },

                        // пересечение параллелепипеда октадерева с помещаемым в него объектом
                        [](const OctreeParallelepiped* p, const TableTriangle* t) -> bool
                        {
                                return shape_intersection(*p, *t);
                        },

                        progress
                );
        // clang-format on
}

VisibleMesh::VisibleMesh(const IObj* obj, double size, const vec3& position, ProgressRatio* progress)
        : m_octree(OCTREE_MAX_DEPTH, OCTREE_MIN_OBJECTS)
{
        double start_time = get_time_seconds();

        create_mesh_object(obj, size, position, progress);

        LOG("Mesh object created, " + to_string_fixed(get_time_seconds() - start_time, 5) + " s");
}

bool VisibleMesh::intersect_approximate(const ray3& r, double* t) const
{
        return m_octree.intersect_root(r, t);
}

bool VisibleMesh::intersect_precise(const ray3& ray, double approximate_t, double* t, const Surface** surface,
                                    const GeometricObject** geometric_object) const
{
        const TableTriangle* triangle;

        if (!m_octree.trace_ray(
                    ray, approximate_t, [&ray, &t, &triangle](const OctreeParallelepiped& parallelepiped,
                                                              const std::vector<const TableTriangle*>& objects) -> bool {
                            return ray_intersection(objects, ray, t, &triangle) && parallelepiped.inside(ray.point(*t));
                    }))
        {
                return false;
        }

        *geometric_object = triangle;
        *surface = this;

        return true;
}

SurfaceProperties VisibleMesh::properties(const vec3& p, const GeometricObject* geometric_object) const
{
        const TableTriangle* triangle = static_cast<const TableTriangle*>(geometric_object);

        SurfaceProperties s = *this;

        s.set_geometric_normal(triangle->geometric_normal());
        s.set_shading_normal(triangle->shading_normal(p));
        s.set_triangle_mesh(true);

        if (triangle->get_material() >= 0)
        {
                const Material& m = m_materials[triangle->get_material()];

                if (triangle->has_texcoord() && m.map_Kd >= 0)
                {
                        s.set_color(m_images[m.map_Kd].get_texture(triangle->texcoord(p)));
                }
                else
                {
                        s.set_color(m.Kd);
                }
        }

        return s;
}
