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

#include "com/log.h"
#include "com/matrix_alg.h"
#include "com/time.h"
#include "com/vec.h"
#include "painter/space/hyperplane_simplex_wrapper.h"
#include "painter/space/ray_intersection.h"

#include <algorithm>
#include <utility>

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
void Mesh<N, T>::create_mesh_object(const Obj<N>* obj, const Matrix<N + 1, N + 1, T>& vertex_matrix, unsigned thread_count,
                                    ProgressRatio* progress)
{
        if (obj->vertices().size() == 0)
        {
                error("No vertices found in obj");
        }

        if (obj->facets().size() == 0)
        {
                error("No facets found in obj");
        }

        m_vertices = to_vector<T>(obj->vertices());
        m_vertices.shrink_to_fit();
        std::transform(m_vertices.begin(), m_vertices.end(), m_vertices.begin(), MatrixMulVector(vertex_matrix));

        m_normals = to_vector<T>(obj->normals());
        m_normals.shrink_to_fit();

        m_texcoords = to_vector<T>(obj->texcoords());
        m_texcoords.shrink_to_fit();

        m_min = Vector<N, T>(limits<T>::max());
        m_max = Vector<N, T>(limits<T>::lowest());
        m_facets.reserve(obj->facets().size());
        for (const typename Obj<N>::Facet& facet : obj->facets())
        {
                m_facets.emplace_back(m_vertices, m_normals, m_texcoords, facet.vertices, facet.has_normal, facet.normals,
                                      facet.has_texcoord, facet.texcoords, facet.material);

                for (int index : facet.vertices)
                {
                        m_min = min_vector(m_min, m_vertices[index]);
                        m_max = max_vector(m_max, m_vertices[index]);
                }
        }

        m_materials.reserve(obj->materials().size());
        for (const typename Obj<N>::Material& m : obj->materials())
        {
                m_materials.emplace_back(m.Kd, m.Ks, m.Ns, m.map_Kd, m.map_Ks);
        }

        m_images.reserve(obj->images().size());
        for (const typename Obj<N>::Image& image : obj->images())
        {
                m_images.emplace_back(image.size, image.srgba_pixels);
        }

        progress->set_text(to_string(1 << N) + "-tree: %v of %m");

        std::vector<HyperplaneSimplexWrapperForShapeIntersection<Facet>> simplex_wrappers;
        simplex_wrappers.reserve(m_facets.size());
        for (const Facet& t : m_facets)
        {
                simplex_wrappers.emplace_back(t);
        }

        // Указатель на объект дерева
        auto lambda_simplex = [w = std::as_const(simplex_wrappers)](int simplex_index) { return &(w[simplex_index]); };

        m_tree.decompose(tree_max_depth<N>(), TREE_MIN_OBJECTS_PER_BOX, m_facets.size(), lambda_simplex, thread_count, progress);
}

template <size_t N, typename T>
Mesh<N, T>::Mesh(const Obj<N>* obj, const Matrix<N + 1, N + 1, T>& vertex_matrix, unsigned thread_count, ProgressRatio* progress)
{
        double start_time = time_in_seconds();

        create_mesh_object(obj, vertex_matrix, thread_count, progress);

        LOG("Mesh object created, " + to_string_fixed(time_in_seconds() - start_time, 5) + " s");
}

template <size_t N, typename T>
bool Mesh<N, T>::intersect_approximate(const Ray<N, T>& r, T* t) const
{
        return m_tree.intersect_root(r, t);
}

template <size_t N, typename T>
bool Mesh<N, T>::intersect_precise(const Ray<N, T>& ray, T approximate_t, T* t, const void** intersection_data) const
{
        const Facet* facet = nullptr;

        if (m_tree.trace_ray(ray, approximate_t,
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
        else
        {
                return false;
        }
}

template <size_t N, typename T>
Vector<N, T> Mesh<N, T>::geometric_normal(const void* intersection_data) const
{
        return static_cast<const Facet*>(intersection_data)->geometric_normal();
}

template <size_t N, typename T>
Vector<N, T> Mesh<N, T>::shading_normal(const Vector<N, T>& p, const void* intersection_data) const
{
        return static_cast<const Facet*>(intersection_data)->shading_normal(p);
}

template <size_t N, typename T>
std::optional<Color> Mesh<N, T>::color(const Vector<N, T>& p, const void* intersection_data) const
{
        const Facet* facet = static_cast<const Facet*>(intersection_data);

        if (facet->material() >= 0)
        {
                const Material& m = m_materials[facet->material()];

                if (facet->has_texcoord() && m.map_Kd >= 0)
                {
                        return m_images[m.map_Kd].texture(facet->texcoord(p));
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

template <size_t N, typename T>
void Mesh<N, T>::min_max(Vector<N, T>* min, Vector<N, T>* max) const
{
        *min = m_min;
        *max = m_max;
}

template class Mesh<3, float>;
template class Mesh<4, float>;
template class Mesh<5, float>;
template class Mesh<6, float>;

template class Mesh<3, double>;
template class Mesh<4, double>;
template class Mesh<5, double>;
template class Mesh<6, double>;
