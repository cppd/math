/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "../space/hyperplane_simplex_wrapper.h"
#include "../space/ray_intersection.h"

#include <src/com/log.h>
#include <src/com/thread.h>
#include <src/com/time.h>
#include <src/com/type/limit.h>
#include <src/numerical/transform.h>
#include <src/numerical/vec.h>

#include <algorithm>
#include <utility>

namespace painter
{
namespace
{
constexpr int TREE_MIN_OBJECTS_PER_BOX = 10;

template <size_t N>
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
}

template <size_t N, typename T>
void MeshObject<N, T>::create_tree(
        const std::vector<Facet>& facets,
        SpatialSubdivisionTree<TreeParallelotope>* tree,
        ProgressRatio* progress)
{
        progress->set_text(to_string(1 << N) + "-tree: %v of %m");

        std::vector<HyperplaneSimplexWrapperForShapeIntersection<Facet>> simplex_wrappers;
        simplex_wrappers.reserve(facets.size());
        for (const Facet& t : facets)
        {
                simplex_wrappers.emplace_back(t);
        }

        // Указатель на объект дерева
        auto lambda_simplex = [w = std::as_const(simplex_wrappers)](int simplex_index) { return &(w[simplex_index]); };

        const unsigned thread_count = hardware_concurrency();
        tree->decompose(
                tree_max_depth<N>(), TREE_MIN_OBJECTS_PER_BOX, facets.size(), lambda_simplex, thread_count, progress);
}

template <size_t N, typename T>
void MeshObject<N, T>::create(const mesh::Reading<N>& mesh_object)
{
        const mesh::Mesh<N>& mesh = mesh_object.mesh();

        if (mesh.vertices.empty())
        {
                error("No vertices found in mesh");
        }

        if (mesh.facets.empty())
        {
                error("No facets found in mesh");
        }

        m_vertices = to_vector<T>(mesh.vertices);
        m_vertices.shrink_to_fit();
        std::transform(
                m_vertices.begin(), m_vertices.end(), m_vertices.begin(),
                matrix::MatrixVectorMultiplier(to_matrix<T>(mesh_object.matrix())));

        m_normals = to_vector<T>(mesh.normals);
        m_normals.shrink_to_fit();

        m_texcoords = to_vector<T>(mesh.texcoords);
        m_texcoords.shrink_to_fit();

        m_min = Vector<N, T>(limits<T>::max());
        m_max = Vector<N, T>(limits<T>::lowest());
        m_facets.reserve(mesh.facets.size());
        bool facets_without_material = false;
        int default_material_index = mesh.materials.size();
        for (const typename mesh::Mesh<N>::Facet& facet : mesh.facets)
        {
                bool no_material = facet.material < 0;
                facets_without_material = facets_without_material || no_material;
                m_facets.emplace_back(
                        m_vertices, m_normals, m_texcoords, facet.vertices, facet.has_normal, facet.normals,
                        facet.has_texcoord, facet.texcoords, no_material ? default_material_index : facet.material);

                for (int index : facet.vertices)
                {
                        m_min = min_vector(m_min, m_vertices[index]);
                        m_max = max_vector(m_max, m_vertices[index]);
                }
        }

        m_materials.reserve(facets_without_material ? mesh.materials.size() + 1 : mesh.materials.size());
        for (const typename mesh::Mesh<N>::Material& m : mesh.materials)
        {
                m_materials.emplace_back(m.Kd, mesh_object.diffuse(), m.map_Kd);
        }
        if (facets_without_material)
        {
                ASSERT(default_material_index == static_cast<int>(m_materials.size()));
                m_materials.emplace_back(mesh_object.color(), mesh_object.diffuse(), -1);
        }

        m_images.reserve(mesh.images.size());
        for (const image::Image<N - 1>& image : mesh.images)
        {
                m_images.emplace_back(image);
        }
}

template <size_t N, typename T>
MeshObject<N, T>::MeshObject(const mesh::Reading<N>& mesh_object, ProgressRatio* progress)
{
        double start_time = time_in_seconds();

        create(mesh_object);
        create_tree(m_facets, &m_tree, progress);

        LOG("Painter mesh object created, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
}

template <size_t N, typename T>
bool MeshObject<N, T>::intersect_approximate(const Ray<N, T>& r, T* t) const
{
        return m_tree.intersect_root(r, t);
}

template <size_t N, typename T>
bool MeshObject<N, T>::intersect_precise(const Ray<N, T>& ray, T approximate_t, T* t, const void** intersection_data)
        const
{
        const Facet* facet = nullptr;

        if (m_tree.trace_ray(
                    ray, approximate_t,
                    // Пересечение луча с набором граней ячейки дерева
                    [&](const std::vector<int>& facet_indices, Vector<N, T>* point) -> bool {
                            if (ray_intersection(m_facets, facet_indices, ray, t, &facet))
                            {
                                    *point = ray.point(*t);
                                    return true;
                            }
                            return false;
                    }))
        {
                *intersection_data = facet;
                return true;
        }

        return false;
}

template <size_t N, typename T>
SurfaceProperties<N, T> MeshObject<N, T>::surface_properties(const Vector<N, T>& p, const void* intersection_data) const
{
        SurfaceProperties<N, T> s;

        const Facet* facet = static_cast<const Facet*>(intersection_data);

        s.set_geometric_normal(facet->geometric_normal());
        s.set_shading_normal(facet->shading_normal(p));

        ASSERT(facet->material() >= 0);

        const Material& m = m_materials[facet->material()];
        s.set_diffuse(m.diffuse);
        if (facet->has_texcoord() && m.map_Kd >= 0)
        {
                s.set_color(m_images[m.map_Kd].texture(facet->texcoord(p)));
        }
        else
        {
                s.set_color(m.Kd);
        }

        return s;
}

template <size_t N, typename T>
void MeshObject<N, T>::min_max(Vector<N, T>* min, Vector<N, T>* max) const
{
        *min = m_min;
        *max = m_max;
}

template class MeshObject<3, float>;
template class MeshObject<4, float>;
template class MeshObject<5, float>;
template class MeshObject<6, float>;

template class MeshObject<3, double>;
template class MeshObject<4, double>;
template class MeshObject<5, double>;
template class MeshObject<6, double>;
}
