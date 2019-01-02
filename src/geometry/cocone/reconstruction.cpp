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

/*
Алгоритмы восстановления поверхностей Cocone и BoundCocone.

По книге

Tamal K. Dey.
Curve and Surface Reconstruction: Algorithms with Mathematical Analysis.
Cambridge University Press, 2007.
*/

#include "reconstruction.h"

#include "extract_manifold.h"
#include "print.h"
#include "prune_facets.h"
#include "structure.h"

#include "com/alg.h"
#include "com/error.h"
#include "com/log.h"
#include "com/names.h"
#include "com/print.h"

#include <unordered_set>

constexpr double RHO_MIN = 0, RHO_MAX = 1;
constexpr double ALPHA_MIN = 0, ALPHA_MAX = 1;

namespace
{
template <size_t N>
bool cocone_facet(const std::vector<ManifoldFacet<N>>& facet_data, int facet)
{
        // Грань считается cocone, если соответствующее ей ребро Вороного пересекает cocone всех N вершин
        for (unsigned v = 0; v < N; ++v)
        {
                if (!facet_data[facet].cocone_vertex[v])
                {
                        return false;
                }
        }
        return true;
}

template <size_t N>
void find_cocone_facets(const std::vector<ManifoldFacet<N>>& facet_data, std::vector<bool>* cocone_facets)
{
        cocone_facets->clear();
        cocone_facets->resize(facet_data.size());

        for (unsigned f = 0; f < facet_data.size(); ++f)
        {
                (*cocone_facets)[f] = cocone_facet(facet_data, f);
        }
}

// В книге это Definition 5.4 (i)
template <size_t N>
bool ratio_condition(const ManifoldVertex<N>& vertex, double rho)
{
        return vertex.radius <= rho * vertex.height;
}

// В книге это Definition 5.4 (ii)
template <size_t N>
bool normal_condition(const ManifoldVertex<N>& v1, const ManifoldVertex<N>& v2, double cos_of_alpha)
{
        double cos_of_angle = dot(v1.positive_norm, v2.positive_norm);

        // Используется абсолютное значение косинуса, так как положительные полюсы
        // могут быть в противоположных направлениях у соседних вершин в зависимости
        // от ситуации с ячейками Вороного.
        return std::abs(cos_of_angle) >= cos_of_alpha;
}

template <size_t N>
void find_interior_vertices(double rho, double cosine_of_alpha, const std::vector<ManifoldVertex<N>>& vertex_data,
                            std::vector<bool>* interior_vertices)
{
        interior_vertices->clear();
        interior_vertices->resize(vertex_data.size(), false);

        int interior_count = 0;

        for (unsigned v = 0; v < vertex_data.size(); ++v)
        {
                const ManifoldVertex<N>& vertex = vertex_data[v];

                if (!ratio_condition(vertex, rho))
                {
                        continue;
                }

                bool flat = true;
                // Нужно соответствие угла со всеми соседними вершинами
                for (int n : vertex.cocone_neighbors)
                {
                        if (!normal_condition(vertex, vertex_data[n], cosine_of_alpha))
                        {
                                flat = false;
                                break;
                        }
                }

                if (flat)
                {
                        (*interior_vertices)[v] = true;
                        ++interior_count;
                }
        }

        LOG("interior points after initial phase: " + to_string(interior_count) + " (" + to_string(vertex_data.size()) + ")");

        if (interior_count == 0)
        {
                return;
        }

        bool found;
        do
        {
                found = false;

                for (unsigned v = 0; v < vertex_data.size(); ++v)
                {
                        if ((*interior_vertices)[v])
                        {
                                continue;
                        }

                        const ManifoldVertex<N>& vertex = vertex_data[v];

                        if (!ratio_condition(vertex, rho))
                        {
                                continue;
                        }

                        for (int n : vertex.cocone_neighbors)
                        {
                                // Достаточно соответствия угла с одной соседней вершиной, являющейся внутренней

                                if (!(*interior_vertices)[n])
                                {
                                        continue;
                                }

                                if (normal_condition(vertex, vertex_data[n], cosine_of_alpha))
                                {
                                        (*interior_vertices)[v] = true;
                                        found = true;
                                        ++interior_count;
                                        break;
                                }
                        }
                }

        } while (found);

        LOG("interior points after expansion phase: " + to_string(interior_count) + " (" + to_string(vertex_data.size()) + ")");
}

template <size_t N>
bool cocone_interior_facet(const std::vector<DelaunayFacet<N>>& delaunay_facets, const std::vector<ManifoldFacet<N>>& facet_data,
                           const std::vector<bool>& interior_vertices, int facet)
{
        // Все вершины грани должны быть interior_cocone или boundary,
        // при этом требуется хотя бы одна interior_cocone.
        bool found = false;
        for (unsigned v = 0; v < N; ++v)
        {
                bool interior = interior_vertices[delaunay_facets[facet].vertices()[v]];

                bool interior_cocone = interior && facet_data[facet].cocone_vertex[v];
                bool boundary = !interior;

                if (!(interior_cocone || boundary))
                {
                        return false;
                }

                found = found || interior_cocone;
        }
        return found;
}

template <size_t N>
void find_cocone_interior_facets(const std::vector<DelaunayFacet<N>>& delaunay_facets,
                                 const std::vector<ManifoldFacet<N>>& facet_data, const std::vector<bool>& interior_vertices,
                                 std::vector<bool>* cocone_facets)
{
        ASSERT(delaunay_facets.size() == facet_data.size());

        cocone_facets->clear();
        cocone_facets->resize(facet_data.size());

        for (unsigned f = 0; f < facet_data.size(); ++f)
        {
                (*cocone_facets)[f] = cocone_interior_facet(delaunay_facets, facet_data, interior_vertices, f);
        }
}

template <size_t N>
void create_normals_and_facets(const std::vector<DelaunayFacet<N>>& delaunay_facets, const std::vector<bool>& cocone_facets,
                               const std::vector<ManifoldVertex<N>>& vertex_data, std::vector<vec<N>>* normals,
                               std::vector<std::array<int, N>>* facets)
{
        std::unordered_set<int> used_points;

        facets->clear();

        for (unsigned i = 0; i < delaunay_facets.size(); ++i)
        {
                if (!cocone_facets[i])
                {
                        continue;
                }

                facets->push_back(delaunay_facets[i].vertices());

                for (int index : delaunay_facets[i].vertices())
                {
                        used_points.insert(index);
                }
        }

        normals->clear();
        normals->resize(vertex_data.size(), vec<N>(0));

        for (int p : used_points)
        {
                (*normals)[p] = vertex_data[p].positive_norm;
        }
}

template <size_t N>
void create_voronoi_delaunay(const std::vector<Vector<N, float>>& source_points, std::vector<vec<N>>* points,
                             std::vector<DelaunayObject<N>>* delaunay_objects, std::vector<DelaunayFacet<N>>* delaunay_facets,
                             ProgressRatio* progress)
{
        std::vector<DelaunaySimplex<N>> delaunay_simplices;
        LOG("compute delaunay...");
        compute_delaunay(source_points, points, &delaunay_simplices, progress);

        LOG("creating delaunay objects and facets and voronoi vertices...");
        create_delaunay_objects_and_facets(*points, delaunay_simplices, delaunay_objects, delaunay_facets);
}

void check_rho_and_aplha(double rho, double alpha)
{
        if (!(rho > RHO_MIN && rho < RHO_MAX))
        {
                std::string interval = "(" + to_string(RHO_MIN) + ", " + to_string(RHO_MAX) + ")";
                error("Rho must be in the interval " + interval + ", but rho = " + to_string(rho, 10));
        }
        if (!(alpha > ALPHA_MIN && alpha < ALPHA_MAX))
        {
                std::string interval = "(" + to_string(ALPHA_MIN) + ", " + to_string(ALPHA_MAX) + ")";
                error("Alpha must be in the interval " + interval + ", but alpha = " + to_string(alpha, 10));
        }
}

template <size_t N>
class ManifoldConstructorImpl : public ManifoldConstructor<N>, public ManifoldConstructorCocone<N>
{
        const bool m_cocone_only;

        std::vector<vec<N>> m_points;
        std::vector<DelaunayObject<N>> m_delaunay_objects;
        std::vector<DelaunayFacet<N>> m_delaunay_facets;
        std::vector<ManifoldVertex<N>> m_vertex_data;
        std::vector<ManifoldFacet<N>> m_facet_data;

        void common_computation(const std::vector<bool>& interior_vertices, std::vector<bool>&& cocone_facets,
                                std::vector<vec<N>>* normals, std::vector<std::array<int, N>>* facets,
                                ProgressRatio* progress) const
        {
                progress->set(1, 4);
                LOG("prune facets...");

                prune_facets_incident_to_sharp_ridges(m_points, m_delaunay_facets, interior_vertices, &cocone_facets);
                if (all_false(cocone_facets))
                {
                        error("Cocone facets not found after prune. " + to_string(N - 1) + "-manifold is not reconstructable.");
                }

                progress->set(2, 4);
                LOG("extract manifold...");

                extract_manifold(m_delaunay_objects, m_delaunay_facets, &cocone_facets);
                if (all_false(cocone_facets))
                {
                        error("Cocone facets not found after manifold extraction. " + to_string(N - 1) +
                              "-manifold is not reconstructable.");
                }

                progress->set(3, 4);
                LOG("create result...");

                create_normals_and_facets(m_delaunay_facets, cocone_facets, m_vertex_data, normals, facets);

                ASSERT(normals->size() == m_points.size());
        }

        void cocone(std::vector<vec<N>>* normals, std::vector<std::array<int, N>>* facets, ProgressRatio* progress) const override
        {
                progress->set_text("Cocone reconstruction: %v of %m");

                progress->set(0, 4);
                LOG("vertex data...");

                std::vector<bool> cocone_facets;
                find_cocone_facets(m_facet_data, &cocone_facets);
                if (all_false(cocone_facets))
                {
                        error("Cocone facets not found. " + to_string(N - 1) + "-manifold is not reconstructable.");
                }

                std::vector<bool> interior_vertices(m_vertex_data.size(), true);

                common_computation(interior_vertices, std::move(cocone_facets), normals, facets, progress);
        }

        // ε-sample EPSILON = 0.1.
        // ρ для отношения ширины и высоты ячейки Вороного, rho = 1.3 * EPSILON.
        // α для углов между векторами к положительным полюсам ячеек Вороного, alpha = 0.14.
        void bound_cocone(double rho, double alpha, std::vector<vec<N>>* normals, std::vector<std::array<int, N>>* facets,
                          ProgressRatio* progress) const override
        {
                if (m_cocone_only)
                {
                        error("Manifold constructor created for Cocone and not for BoundCocone");
                }

                check_rho_and_aplha(rho, alpha);

                progress->set_text("BoundCocone reconstruction: %v of %m");

                progress->set(0, 4);
                LOG("vertex data...");

                std::vector<bool> interior_vertices;
                find_interior_vertices(rho, std::cos(alpha), m_vertex_data, &interior_vertices);
                if (all_false(interior_vertices))
                {
                        error("Interior vertices not found. " + to_string(N - 1) + "-manifold is not reconstructable.");
                }

                std::vector<bool> cocone_facets;
                find_cocone_interior_facets(m_delaunay_facets, m_facet_data, interior_vertices, &cocone_facets);
                if (all_false(cocone_facets))
                {
                        error("Cocone interior facets not found. " + to_string(N - 1) + "-manifold is not reconstructable.");
                }

                common_computation(interior_vertices, std::move(cocone_facets), normals, facets, progress);
        }

        std::vector<std::array<int, N + 1>> delaunay_objects() const override
        {
                std::vector<std::array<int, N + 1>> objects;
                objects.reserve(m_delaunay_objects.size());
                for (const DelaunayObject<N>& d : m_delaunay_objects)
                {
                        objects.push_back(d.vertices());
                }
                return objects;
        }

public:
        ManifoldConstructorImpl(const std::vector<Vector<N, float>>& source_points, bool cocone_only, ProgressRatio* progress)
                : m_cocone_only(cocone_only)
        {
                // Проверить на самый минимум по количеству точек
                if (source_points.size() < N + 2)
                {
                        error("Error point count " + to_string(source_points.size()) + " for cocone manifold reconstruction in " +
                              space_name(N));
                }

                progress->set_text("Voronoi-Delaunay: %v of %m");

                create_voronoi_delaunay(source_points, &m_points, &m_delaunay_objects, &m_delaunay_facets, progress);

                vertex_and_facet_data(!cocone_only, m_points, m_delaunay_objects, m_delaunay_facets, &m_vertex_data,
                                      &m_facet_data);

                ASSERT(source_points.size() == m_points.size());
        }
};
}

template <size_t N>
std::unique_ptr<ManifoldConstructor<N>> create_manifold_constructor(const std::vector<Vector<N, float>>& source_points,
                                                                    ProgressRatio* progress)
{
        return std::make_unique<ManifoldConstructorImpl<N>>(source_points, false, progress);
}

template <size_t N>
std::unique_ptr<ManifoldConstructorCocone<N>> create_manifold_constructor_cocone(
        const std::vector<Vector<N, float>>& source_points, ProgressRatio* progress)
{
        return std::make_unique<ManifoldConstructorImpl<N>>(source_points, true, progress);
}

// clang-format off
template
std::unique_ptr<ManifoldConstructor<2>> create_manifold_constructor(const std::vector<Vector<2, float>>& source_points,
                                                                    ProgressRatio* progress);
template
std::unique_ptr<ManifoldConstructor<3>> create_manifold_constructor(const std::vector<Vector<3, float>>& source_points,
                                                                    ProgressRatio* progress);
template
std::unique_ptr<ManifoldConstructor<4>> create_manifold_constructor(const std::vector<Vector<4, float>>& source_points,
                                                                    ProgressRatio* progress);
template
std::unique_ptr<ManifoldConstructor<5>> create_manifold_constructor(const std::vector<Vector<5, float>>& source_points,
                                                                    ProgressRatio* progress);
template
std::unique_ptr<ManifoldConstructorCocone<2>> create_manifold_constructor_cocone(
        const std::vector<Vector<2, float>>& source_points, ProgressRatio* progress);
template
std::unique_ptr<ManifoldConstructorCocone<3>> create_manifold_constructor_cocone(
        const std::vector<Vector<3, float>>& source_points, ProgressRatio* progress);
template
std::unique_ptr<ManifoldConstructorCocone<4>> create_manifold_constructor_cocone(
        const std::vector<Vector<4, float>>& source_points, ProgressRatio* progress);
template
std::unique_ptr<ManifoldConstructorCocone<5>> create_manifold_constructor_cocone(
        const std::vector<Vector<5, float>>& source_points, ProgressRatio* progress);
// clang-format on
