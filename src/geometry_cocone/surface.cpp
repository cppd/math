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

/*
Алгоритмы восстановления поверхностей COCONE и BOUND COCONE.

По книге

Tamal K. Dey.
Curve and Surface Reconstruction: Algorithms with Mathematical Analysis.
Cambridge University Press, 2007.
*/

#include "surface.h"

#include "extract_manifold.h"
#include "print.h"
#include "prune_facets.h"
#include "structure.h"

#include "com/alg.h"
#include "com/error.h"
#include "com/log.h"
#include "com/print.h"

#include <unordered_set>

namespace
{
template <size_t N>
void find_cocone_facets(const std::vector<SurfaceFacet<N>>& facet_data, std::vector<bool>* cocone_facets)
{
        cocone_facets->clear();
        cocone_facets->resize(facet_data.size());

        bool found = false;
        for (unsigned f = 0; f < facet_data.size(); ++f)
        {
                // Грань считается COCONE, если соответствующее ей ребро Вороного пересекает COCONE всех N вершин
                bool cocone = all_true(facet_data[f].cocone_vertex);
                (*cocone_facets)[f] = cocone;
                found = found || cocone;
        }

        if (!found)
        {
                error("Cocone facets not found. Surface is not reconstructable.");
        }
}

template <size_t N>
void find_interior_vertices(double RHO, double ALPHA, const std::vector<SurfaceVertex<N>>& vertex_data,
                            std::vector<bool>* interior_vertices)
{
        interior_vertices->clear();
        interior_vertices->resize(vertex_data.size(), false);

        int interior_count = 0;

        for (unsigned v = 0; v < vertex_data.size(); ++v)
        {
                const SurfaceVertex<N>& data = vertex_data[v];

                if (!(data.radius <= RHO * data.height))
                {
                        continue;
                }

                bool flat = true;

                // Нужно соответствие угла со всеми соседними вершинами
                for (int n : data.cocone_neighbors)
                {
                        if (!(dot(data.positive_norm, vertex_data[n].positive_norm) >= ALPHA))
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

        if (interior_count == 0)
        {
                error("interior points not found");
        }

        LOG("interior points after initial phase: " + to_string(interior_count) + " (" + to_string(vertex_data.size()) + ")");

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

                        const SurfaceVertex<N>& data = vertex_data[v];

                        if (!(data.radius <= RHO * data.height))
                        {
                                continue;
                        }

                        for (int n : data.cocone_neighbors)
                        {
                                // Достаточно соответствия угла с одной соседней вершиной, являющейся внутренней

                                if (!(*interior_vertices)[n])
                                {
                                        continue;
                                }

                                if (dot(data.positive_norm, vertex_data[n].positive_norm) >= ALPHA)
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
void find_cocone_interior_facets(const std::vector<DelaunayFacet<N>>& delaunay_facets,
                                 const std::vector<SurfaceFacet<N>>& facet_data, const std::vector<bool>& interior_vertices,
                                 std::vector<bool>* cocone_facets)
{
        ASSERT(delaunay_facets.size() == facet_data.size());

        cocone_facets->clear();
        cocone_facets->resize(delaunay_facets.size());

        for (unsigned f = 0; f < facet_data.size(); ++f)
        {
                bool cocone = true;
                bool interior_found = false;
                for (unsigned v = 0; v < N; ++v)
                {
                        bool interior_cocone =
                                interior_vertices[delaunay_facets[f].get_vertices()[v]] && facet_data[f].cocone_vertex[v];
                        bool boundary = !interior_vertices[delaunay_facets[f].get_vertices()[v]];
                        if (!(interior_cocone || boundary))
                        {
                                cocone = false;
                                break;
                        }
                        if (interior_cocone)
                        {
                                interior_found = true;
                        }
                }
                (*cocone_facets)[f] = interior_found && cocone;
        }

        if (all_false(*cocone_facets))
        {
                error("Cocone interior facets not found. Surface is not reconstructable.");
        }
}

template <size_t N>
void create_normals_and_facets(const std::vector<DelaunayFacet<N>>& delaunay_facets, const std::vector<bool>& cocone_facets,
                               const std::vector<SurfaceVertex<N>>& vertex_data, std::vector<vec<N>>* normals,
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

                facets->push_back(delaunay_facets[i].get_vertices());

                for (int index : delaunay_facets[i].get_vertices())
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

template <size_t N>
class SurfaceReconstructor : public ISurfaceReconstructor<N>, public ISurfaceReconstructorCoconeOnly<N>
{
        const bool m_cocone_only;

        std::vector<vec<N>> m_points;
        std::vector<DelaunayObject<N>> m_delaunay_objects;
        std::vector<DelaunayFacet<N>> m_delaunay_facets;
        std::vector<SurfaceVertex<N>> m_vertex_data;
        std::vector<SurfaceFacet<N>> m_facet_data;

        void common_computation(const std::vector<bool>& interior_vertices, std::vector<bool>&& cocone_facets,
                                std::vector<vec<N>>* normals, std::vector<std::array<int, N>>* facets,
                                ProgressRatio* progress) const
        {
                progress->set(1, 4);
                LOG("prune facets...");

                prune_facets_incident_to_sharp_ridges(m_points, m_delaunay_facets, interior_vertices, &cocone_facets);

                if (all_false(cocone_facets))
                {
                        error("Cocone facets not found after prune");
                }

                //

                progress->set(2, 4);
                LOG("extract manifold...");

                extract_manifold(m_delaunay_objects, m_delaunay_facets, &cocone_facets);

                if (all_false(cocone_facets))
                {
                        error("Cocone facets not found after manifold extraction");
                }

                //

                progress->set(3, 4);
                LOG("create result...");

                create_normals_and_facets(m_delaunay_facets, cocone_facets, m_vertex_data, normals, facets);

                ASSERT(normals->size() == m_points.size());
        }

        void cocone(std::vector<vec<N>>* normals, std::vector<std::array<int, N>>* facets, ProgressRatio* progress) const override
        {
                progress->set_text("COCONE reconstruction: %v of %m");

                progress->set(0, 4);
                LOG("vertex data...");

                std::vector<bool> cocone_facets;
                std::vector<bool> interior_vertices;

                find_cocone_facets(m_facet_data, &cocone_facets);
                interior_vertices.resize(m_vertex_data.size(), true);

                common_computation(interior_vertices, std::move(cocone_facets), normals, facets, progress);
        }

        // ε-sample EPSILON = 0.1;
        // ρ для отношения ширины и высоты ячейки Вороного
        // RHO = 1.3 * EPSILON;
        // α для углов между векторами к положительным полюсам ячеек Вороного
        // ALPHA = 0.14;
        void bound_cocone(double RHO, double ALPHA, std::vector<vec<N>>* normals, std::vector<std::array<int, N>>* facets,
                          ProgressRatio* progress) const override
        {
                if (m_cocone_only)
                {
                        error("Surface reconstructor created for cocone and not for bound cocone");
                }

                progress->set_text("BOUND COCONE reconstruction: %v of %m");

                progress->set(0, 4);
                LOG("vertex data...");

                std::vector<bool> cocone_facets;
                std::vector<bool> interior_vertices;

                find_interior_vertices(RHO, ALPHA, m_vertex_data, &interior_vertices);
                find_cocone_interior_facets(m_delaunay_facets, m_facet_data, interior_vertices, &cocone_facets);

                common_computation(interior_vertices, std::move(cocone_facets), normals, facets, progress);
        }

public:
        SurfaceReconstructor(const std::vector<Vector<N, float>>& source_points, bool cocone_only, ProgressRatio* progress)
                : m_cocone_only(cocone_only)
        {
                // Проверить на самый минимум по количеству точек
                if (source_points.size() < N + 2)
                {
                        error("Error point count " + std::to_string(source_points.size()) +
                              " for cocone manifold reconstruction " + std::to_string(N) + "D");
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
std::unique_ptr<ISurfaceReconstructor<N>> create_surface_reconstructor(const std::vector<Vector<N, float>>& source_points,
                                                                       ProgressRatio* progress)
{
        return std::make_unique<SurfaceReconstructor<N>>(source_points, false, progress);
}

template <size_t N>
std::unique_ptr<ISurfaceReconstructorCoconeOnly<N>> create_surface_reconstructor_cocone_only(
        const std::vector<Vector<N, float>>& source_points, ProgressRatio* progress)
{
        return std::make_unique<SurfaceReconstructor<N>>(source_points, true, progress);
}

// clang-format off
template
std::unique_ptr<ISurfaceReconstructor<2>> create_surface_reconstructor(const std::vector<Vector<2, float>>& source_points,
                                                                       ProgressRatio* progress);
template
std::unique_ptr<ISurfaceReconstructor<3>> create_surface_reconstructor(const std::vector<Vector<3, float>>& source_points,
                                                                       ProgressRatio* progress);
template
std::unique_ptr<ISurfaceReconstructorCoconeOnly<2>> create_surface_reconstructor_cocone_only(
        const std::vector<Vector<2, float>>& source_points, ProgressRatio* progress);
template
std::unique_ptr<ISurfaceReconstructorCoconeOnly<3>> create_surface_reconstructor_cocone_only(
        const std::vector<Vector<3, float>>& source_points, ProgressRatio* progress);
// clang-format on
