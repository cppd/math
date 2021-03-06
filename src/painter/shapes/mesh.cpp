/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/com/log.h>
#include <src/com/thread.h>
#include <src/com/time.h>
#include <src/com/type/limit.h>
#include <src/geometry/spatial/shape_intersection.h>
#include <src/numerical/transform.h>
#include <src/numerical/vec.h>

#include <algorithm>
#include <utility>

namespace ns::painter
{
namespace
{
constexpr int TREE_MIN_OBJECTS_PER_BOX = 10;

template <std::size_t N>
int tree_max_depth()
{
        static_assert(N >= 3);

        switch (N)
        {
        case 3:
                return 10;
        case 4:
                return 8;
        case 5:
                return 6;
        case 6:
                return 5;
        default:
                // Сумма геометрической прогрессии s = (pow(r, n) - 1) / (r - 1).
                // Для s и r найти n = log(s * (r - 1) + 1) / log(r).
                double s = 1e9;
                double r = std::pow(2, N);
                double n = std::log(s * (r - 1) + 1) / std::log(r);
                return std::max(2.0, std::floor(n));
        }
}

template <std::size_t N>
std::array<int, N> add_offset(const std::array<int, N>& src, int offset, bool add)
{
        std::array<int, N> r;
        if (add)
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        r[i] = offset + src[i];
                }
        }
        else
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        r[i] = -1;
                }
        }
        return r;
}

template <std::size_t N>
std::array<int, N> add_offset(const std::array<int, N>& src, int offset)
{
        std::array<int, N> r;
        for (unsigned i = 0; i < N; ++i)
        {
                r[i] = offset + src[i];
        }
        return r;
}
}

template <std::size_t N, typename T>
void Mesh<N, T>::create(const mesh::Reading<N>& mesh_object)
{
        const mesh::Mesh<N>& mesh = mesh_object.mesh();

        if (mesh.vertices.empty())
        {
                return;
        }
        if (mesh.facets.empty())
        {
                return;
        }

        int vertices_offset = m_vertices.size();
        int normals_offset = m_normals.size();
        int texcoords_offset = m_texcoords.size();
        int materials_offset = m_materials.size();
        int images_offset = m_images.size();

        {
                const std::vector<Vector<N, T>>& vertices = to_vector<T>(mesh.vertices);
                m_vertices.insert(m_vertices.cend(), vertices.cbegin(), vertices.cend());
        }
        {
                auto iter_begin = std::next(m_vertices.begin(), vertices_offset);
                auto iter_end = m_vertices.end();
                std::transform(
                        iter_begin, iter_end, iter_begin,
                        matrix::MatrixVectorMultiplier(to_matrix<T>(mesh_object.matrix())));
        }
        {
                const std::vector<Vector<N, T>>& normals = to_vector<T>(mesh.normals);
                m_normals.insert(m_normals.cend(), normals.cbegin(), normals.cend());
        }
        {
                const std::vector<Vector<N - 1, T>>& texcoords = to_vector<T>(mesh.texcoords);
                m_texcoords.insert(m_texcoords.cend(), texcoords.cbegin(), texcoords.cend());
        }

        bool facets_without_material = false;
        int default_material_index = mesh.materials.size();
        for (const typename mesh::Mesh<N>::Facet& facet : mesh.facets)
        {
                bool no_material = facet.material < 0;
                facets_without_material = facets_without_material || no_material;
                int facet_material = no_material ? default_material_index : facet.material;

                std::array<int, N> vertices = add_offset(facet.vertices, vertices_offset);
                std::array<int, N> normals = add_offset(facet.normals, normals_offset, facet.has_normal);
                std::array<int, N> texcoords = add_offset(facet.texcoords, texcoords_offset, facet.has_texcoord);
                int material = facet_material + materials_offset;

                m_facets.emplace_back(
                        m_vertices, m_normals, m_texcoords, vertices, facet.has_normal, normals, facet.has_texcoord,
                        texcoords, material);
        }

        m_alpha = std::clamp(mesh_object.alpha(), Color::DataType(0), Color::DataType(1));

        for (const typename mesh::Mesh<N>::Material& m : mesh.materials)
        {
                int map_Kd = m.map_Kd < 0 ? -1 : (images_offset + m.map_Kd);
                m_materials.emplace_back(mesh_object.metalness(), mesh_object.roughness(), m.Kd, map_Kd);
        }
        if (facets_without_material)
        {
                ASSERT(materials_offset + default_material_index == static_cast<int>(m_materials.size()));
                m_materials.emplace_back(mesh_object.metalness(), mesh_object.roughness(), mesh_object.color(), -1);
        }

        for (const image::Image<N - 1>& image : mesh.images)
        {
                m_images.emplace_back(image);
        }
}

template <std::size_t N, typename T>
void Mesh<N, T>::create(const std::vector<mesh::Reading<N>>& mesh_objects)
{
        if (mesh_objects.empty())
        {
                error("No objects to paint");
        }

        m_vertices.clear();
        m_normals.clear();
        m_texcoords.clear();
        m_materials.clear();
        m_images.clear();
        m_facets.clear();
        std::size_t vertex_count = 0;
        std::size_t normal_count = 0;
        std::size_t texcoord_count = 0;
        std::size_t material_count = 0;
        std::size_t image_count = 0;
        std::size_t facet_count = 0;
        for (const mesh::Reading<N>& mesh : mesh_objects)
        {
                vertex_count += mesh.mesh().vertices.size();
                normal_count += mesh.mesh().normals.size();
                texcoord_count += mesh.mesh().texcoords.size();
                bool no_material = false;
                for (const typename mesh::Mesh<N>::Facet& facet : mesh.mesh().facets)
                {
                        if (facet.material < 0)
                        {
                                no_material = true;
                                break;
                        }
                }
                material_count += mesh.mesh().materials.size() + (no_material ? 1 : 0);
                image_count += mesh.mesh().images.size();
                facet_count += mesh.mesh().facets.size();
        }
        m_vertices.reserve(vertex_count);
        m_normals.reserve(normal_count);
        m_texcoords.reserve(texcoord_count);
        m_materials.reserve(material_count);
        m_images.reserve(image_count);
        m_facets.reserve(facet_count);

        for (const mesh::Reading<N>& mesh_object : mesh_objects)
        {
                create(mesh_object);
        }

        if (m_vertices.empty())
        {
                error("No vertices found in meshes");
        }
        if (m_facets.empty())
        {
                error("No facets found in meshes");
        }

        ASSERT(vertex_count == m_vertices.size());
        ASSERT(normal_count == m_normals.size());
        ASSERT(texcoord_count == m_texcoords.size());
        ASSERT(material_count == m_materials.size());
        ASSERT(image_count == m_images.size());
        ASSERT(facet_count == m_facets.size());
}

template <std::size_t N, typename T>
Mesh<N, T>::Mesh(const std::vector<const mesh::MeshObject<N>*>& mesh_objects, ProgressRatio* progress)
{
        TimePoint start_time = time();

        {
                std::vector<mesh::Reading<N>> reading;
                reading.reserve(mesh_objects.size());
                for (const mesh::MeshObject<N>* mesh_object : mesh_objects)
                {
                        reading.emplace_back(*mesh_object);
                }
                create(reading);
        }

        progress->set_text(to_string(1 << N) + "-tree: %v of %m");

        m_tree.emplace(m_facets, tree_max_depth<N>(), TREE_MIN_OBJECTS_PER_BOX, progress);

        LOG("Painter mesh object created, " + to_string_fixed(duration_from(start_time), 5) + " s");
}

template <std::size_t N, typename T>
std::optional<T> Mesh<N, T>::intersect_bounding(const Ray<N, T>& r) const
{
        return m_tree->intersect_root(r);
}

template <std::size_t N, typename T>
std::optional<Intersection<N, T>> Mesh<N, T>::intersect(const Ray<N, T>& ray, T bounding_distance) const
{
        std::optional<std::tuple<T, const MeshFacet<N, T>*>> v = m_tree->intersect(ray, bounding_distance);
        if (!v)
        {
                return std::nullopt;
        }
        Intersection<N, T> result;
        result.distance = std::get<0>(*v);
        result.data = std::get<1>(*v);
        result.surface = this;
        return result;
}

template <std::size_t N, typename T>
SurfaceProperties<N, T> Mesh<N, T>::properties(const Vector<N, T>& p, const void* intersection_data) const
{
        SurfaceProperties<N, T> s;

        const MeshFacet<N, T>* facet = static_cast<const MeshFacet<N, T>*>(intersection_data);

        s.set_geometric_normal(facet->geometric_normal());
        s.set_shading_normal(facet->shading_normal(p));
        s.set_alpha(m_alpha);

        return s;
}

template <std::size_t N, typename T>
Color Mesh<N, T>::lighting(
        const Vector<N, T>& p,
        const void* intersection_data,
        const Vector<N, T>& n,
        const Vector<N, T>& v,
        const Vector<N, T>& l) const
{
        const MeshFacet<N, T>* facet = static_cast<const MeshFacet<N, T>*>(intersection_data);

        ASSERT(facet->material() >= 0);

        const Material& m = m_materials[facet->material()];

        Color color;
        if (facet->has_texcoord() && m.map_Kd >= 0)
        {
                color = m_images[m.map_Kd].color(facet->texcoord(p));
        }
        else
        {
                color = m.Kd;
        }

        return m_shading.direct_lighting(m.metalness, m.roughness, color, n, v, l);
}

template <std::size_t N, typename T>
SurfaceReflection<N, T> Mesh<N, T>::reflection(
        RandomEngine<T>& random_engine,
        const Vector<N, T>& p,
        const void* intersection_data,
        const Vector<N, T>& n,
        const Vector<N, T>& v) const
{
        const MeshFacet<N, T>* facet = static_cast<const MeshFacet<N, T>*>(intersection_data);

        ASSERT(facet->material() >= 0);

        const Material& m = m_materials[facet->material()];

        Color color;
        if (facet->has_texcoord() && m.map_Kd >= 0)
        {
                color = m_images[m.map_Kd].color(facet->texcoord(p));
        }
        else
        {
                color = m.Kd;
        }

        return m_shading.reflection(random_engine, m.metalness, m.roughness, color, n, v);
}

template <std::size_t N, typename T>
geometry::BoundingBox<N, T> Mesh<N, T>::bounding_box() const
{
        return m_tree->bounding_box();
}

template <std::size_t N, typename T>
std::function<bool(const geometry::ShapeWrapperForIntersection<geometry::ParallelotopeAA<N, T>>&)> Mesh<N, T>::
        intersection_function() const
{
        return [w = geometry::ShapeWrapperForIntersection(m_tree->root())](
                       const geometry::ShapeWrapperForIntersection<geometry::ParallelotopeAA<N, T>>& p)
        {
                return geometry::shape_intersection(w, p);
        };
}

template class Mesh<3, float>;
template class Mesh<4, float>;
template class Mesh<5, float>;
template class Mesh<6, float>;

template class Mesh<3, double>;
template class Mesh<4, double>;
template class Mesh<5, double>;
template class Mesh<6, double>;
}
