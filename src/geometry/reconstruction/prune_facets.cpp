/*
Copyright (C) 2017-2024 Topological Manifold

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
Tamal K. Dey.
Curve and Surface Reconstruction: Algorithms with Mathematical Analysis.
Cambridge University Press, 2007.

4.1.3 Pruning
*/

#include "prune_facets.h"

#include "../core/delaunay.h"
#include "../core/ridge.h"

#include <src/com/error.h>
#include <src/numerical/complement.h>
#include <src/numerical/vector.h>
#include <src/settings/instantiation.h>

#include <array>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ns::geometry::reconstruction
{
namespace
{
template <std::size_t N>
using RidgeMap = std::unordered_map<core::Ridge<N>, core::RidgeFacets<core::DelaunayFacet<N>>>;

template <std::size_t N>
using RidgeSet = std::unordered_set<core::Ridge<N>>;

// orthonormal orthogonal complement of a ridge
template <std::size_t N, typename T>
class RidgeComplement final
{
        static_assert(N >= 2);

        // e0 = unit orthogonal complement of n-1 points and a point.
        // e1 = unit orthogonal complement of n-1 points and e0.
        Vector<N, T> e0_;
        Vector<N, T> e1_;

public:
        RidgeComplement(const std::vector<Vector<N, T>>& points, const std::array<int, N - 1>& indices, const int point)
        {
                std::array<Vector<N, T>, N - 1> vectors;
                for (std::size_t i = 0; i < N - 2; ++i)
                {
                        vectors[i] = points[indices[i + 1]] - points[indices[0]];
                }

                vectors[N - 2] = points[point] - points[indices[0]];
                e0_ = numerical::orthogonal_complement(vectors).normalized();

                vectors[N - 2] = e0_;
                e1_ = numerical::orthogonal_complement(vectors).normalized();
        }

        [[nodiscard]] Vector<2, T> coordinates(const Vector<N, T>& v) const
        {
                return Vector<2, T>(dot(e0_, v), dot(e1_, v)).normalized();
        }
};

template <std::size_t N>
[[nodiscard]] bool boundary_ridge(const std::vector<bool>& interior_vertices, const core::Ridge<N>& ridge)
{
        for (const auto v : ridge.vertices())
        {
                if (!interior_vertices[v])
                {
                        return true;
                }
        }
        return false;
}

template <typename T>
struct Angles final
{
        T cos_plus = 1;
        T cos_minus = 1;
        T sin_plus = 0;
        T sin_minus = 0;
};

template <std::size_t N, typename T>
[[nodiscard]] Angles<T> compute_angles(
        const std::vector<Vector<N, T>>& points,
        const core::Ridge<N>& ridge,
        const core::RidgeFacets<core::DelaunayFacet<N>>& facets)
{
        ASSERT(!facets.empty());

        const RidgeComplement basis(points, ridge.vertices(), facets.cbegin()->point());

        const Vector<2, T> base = basis.coordinates(points[facets.cbegin()->point()] - points[ridge.vertices()[0]]);
        ASSERT(is_finite(base));

        Angles<T> res;

        for (auto facet = std::next(facets.cbegin()); facet != facets.cend(); ++facet)
        {
                const Vector<2, T> v = basis.coordinates(points[facet->point()] - points[ridge.vertices()[0]]);
                ASSERT(is_finite(v));

                const T sine = cross(base, v);
                const T cosine = dot(base, v);

                if (sine >= 0)
                {
                        if (cosine < res.cos_plus)
                        {
                                res.cos_plus = cosine;
                                res.sin_plus = sine;
                        }
                }
                else
                {
                        if (cosine < res.cos_minus)
                        {
                                res.cos_minus = cosine;
                                res.sin_minus = sine;
                        }
                }
        }

        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] bool sharp_ridge(
        const std::vector<Vector<N, T>>& points,
        const std::vector<bool>& interior_vertices,
        const core::Ridge<N>& ridge,
        const core::RidgeFacets<core::DelaunayFacet<N>>& ridge_facets)
{
        ASSERT(!ridge_facets.empty());

        if (boundary_ridge(interior_vertices, ridge))
        {
                return false;
        }

        if (ridge_facets.size() == 1)
        {
                // sharp by default
                return true;
        }

        const Angles<T> angles = compute_angles(points, ridge, ridge_facets);

        // not sharp if any angle is greater than or equal to 90 degrees
        if (angles.cos_plus <= 0 || angles.cos_minus <= 0)
        {
                return false;
        }

        // if two angles are less than 90 degrees, then their sum is less 180 degrees
        // cos(a + b) = cos(a)cos(b) - sin(a)sin(b)
        // sin_minus <= 0, so use its absolute value
        const T cos_a_plus_b = angles.cos_plus * angles.cos_minus - std::abs(angles.sin_plus * angles.sin_minus);

        // sharp if the sum is less than 90 degrees
        return cos_a_plus_b > 0;
}

template <std::size_t N>
[[nodiscard]] RidgeSet<N> prune(
        const std::vector<Vector<N, double>>& points,
        const std::vector<bool>& interior_vertices,
        const std::unordered_map<const core::DelaunayFacet<N>*, int>& facet_ptr_index,
        const RidgeSet<N>& suspicious_ridges,
        std::vector<bool>* const cocone_facets,
        RidgeMap<N>* const ridge_map)
{
        RidgeSet<N> ridges;

        for (const core::Ridge<N>& r : suspicious_ridges)
        {
                const auto ridge_iter = ridge_map->find(r);
                if (ridge_iter == ridge_map->cend())
                {
                        continue;
                }

                if (!sharp_ridge(points, interior_vertices, ridge_iter->first, ridge_iter->second))
                {
                        continue;
                }

                std::vector<const core::DelaunayFacet<N>*> facets_to_remove;
                facets_to_remove.reserve(ridge_iter->second.size());

                for (auto d = ridge_iter->second.cbegin(); d != ridge_iter->second.cend(); ++d)
                {
                        core::add_to_ridges(*(d->facet()), d->point(), &ridges);
                        facets_to_remove.push_back(d->facet());

                        const auto del = facet_ptr_index.find(d->facet());
                        ASSERT(del != facet_ptr_index.cend());
                        (*cocone_facets)[del->second] = false;
                }

                for (const core::DelaunayFacet<N>* const facet : facets_to_remove)
                {
                        core::remove_from_ridges(facet, ridge_map);
                }
        }

        return ridges;
}
}

template <std::size_t N>
void prune_facets_incident_to_sharp_ridges(
        const std::vector<Vector<N, double>>& points,
        const std::vector<core::DelaunayFacet<N>>& delaunay_facets,
        const std::vector<bool>& interior_vertices,
        std::vector<bool>* const cocone_facets)
{
        ASSERT(!delaunay_facets.empty() && delaunay_facets.size() == cocone_facets->size());
        ASSERT(points.size() == interior_vertices.size());

        RidgeMap<N> ridge_map;
        std::unordered_map<const core::DelaunayFacet<N>*, int> facet_ptr_index;
        for (std::size_t i = 0; i < delaunay_facets.size(); ++i)
        {
                if ((*cocone_facets)[i])
                {
                        core::add_to_ridges(&delaunay_facets[i], &ridge_map);
                        facet_ptr_index.emplace(&delaunay_facets[i], i);
                }
        }

        RidgeSet<N> suspicious_ridges(ridge_map.size());
        for (const auto& e : ridge_map)
        {
                suspicious_ridges.insert(e.first);
        }

        while (!suspicious_ridges.empty())
        {
                suspicious_ridges =
                        prune(points, interior_vertices, facet_ptr_index, suspicious_ridges, cocone_facets, &ridge_map);
        }
}

#define TEMPLATE(N)                                                                                    \
        template void prune_facets_incident_to_sharp_ridges(                                           \
                const std::vector<Vector<(N), double>>&, const std::vector<core::DelaunayFacet<(N)>>&, \
                const std::vector<bool>&, std::vector<bool>*);

TEMPLATE_INSTANTIATION_N_2(TEMPLATE)
}
