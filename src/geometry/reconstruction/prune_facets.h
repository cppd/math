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

/*
Tamal K. Dey.
Curve and Surface Reconstruction: Algorithms with Mathematical Analysis.
Cambridge University Press, 2007.

4.1.3 Pruning
*/

#pragma once

#include "../core/delaunay.h"
#include "../core/ridge.h"

#include <src/com/error.h>
#include <src/numerical/complement.h>
#include <src/numerical/vec.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ns::geometry
{
namespace prune_facets_implementation
{
template <std::size_t N>
using RidgeData = RidgeDataN<DelaunayFacet<N>>;
template <std::size_t N>
using RidgeMap = std::unordered_map<Ridge<N>, RidgeData<N>>;
template <std::size_t N>
using RidgeSet = std::unordered_set<Ridge<N>>;

// e1 = unit orthogonal complement of n-1 points and a point.
// e2 = unit orthogonal complement of n-1 points and e1.
template <std::size_t N, typename T>
void ortho_e0_e1(
        const std::vector<Vector<N, T>>& points,
        const std::array<int, N - 1>& indices,
        int point,
        Vector<N, T>* e1,
        Vector<N, T>* e2)
{
        static_assert(N > 1);

        std::array<Vector<N, T>, N - 1> vectors;

        for (unsigned i = 0; i < N - 2; ++i)
        {
                vectors[i] = points[indices[i + 1]] - points[indices[0]];
        }

        vectors[N - 2] = points[point] - points[indices[0]];

        *e1 = numerical::orthogonal_complement(vectors).normalized();

        vectors[N - 2] = *e1;

        *e2 = numerical::orthogonal_complement(vectors).normalized();
}

template <std::size_t N>
bool boundary_ridge(const std::vector<bool>& interior_vertices, const Ridge<N>& ridge)
{
        for (int v : ridge.vertices())
        {
                if (!interior_vertices[v])
                {
                        return true;
                }
        }
        return false;
}

template <std::size_t N>
bool sharp_ridge(
        const std::vector<Vector<N, double>>& points,
        const std::vector<bool>& interior_vertices,
        const Ridge<N>& ridge,
        const RidgeData<N>& ridge_data)
{
        ASSERT(!ridge_data.empty());

        if (boundary_ridge(interior_vertices, ridge))
        {
                return false;
        }

        if (ridge_data.size() == 1)
        {
                // sharp by default
                return true;
        }

        // orthonormal orthogonal complement of ridge
        Vector<N, double> e0;
        Vector<N, double> e1;
        ortho_e0_e1(points, ridge.vertices(), ridge_data.cbegin()->point(), &e0, &e1);

        const auto e0_e1 = [&e0, &e1](const Vector<N, double>& v)
        {
                return Vector<2, double>(dot(e0, v), dot(e1, v)).normalized();
        };

        Vector<2, double> base = e0_e1(points[ridge_data.cbegin()->point()] - points[ridge.vertices()[0]]);
        ASSERT(is_finite(base));

        double cos_plus = 1;
        double cos_minus = 1;
        double sin_plus = 0;
        double sin_minus = 0;

        for (auto ridge_facet = std::next(ridge_data.cbegin()); ridge_facet != ridge_data.cend(); ++ridge_facet)
        {
                Vector<2, double> v = e0_e1(points[ridge_facet->point()] - points[ridge.vertices()[0]]);
                ASSERT(is_finite(v));

                double sine = cross(base, v);
                double cosine = dot(base, v);

                if (sine >= 0)
                {
                        if (cosine < cos_plus)
                        {
                                cos_plus = cosine;
                                sin_plus = sine;
                        }
                }
                else
                {
                        if (cosine < cos_minus)
                        {
                                cos_minus = cosine;
                                sin_minus = sine;
                        }
                }
        }

        // not sharp if any angle is greater than or equal to 90 degree
        if (cos_plus <= 0 || cos_minus <= 0)
        {
                return false;
        }

        // if two angles are less than 90 degrees, then their sum is less 180 degrees
        // cos(a + b) = cos(a)cos(b) - sin(a)sin(b).
        // sin_minus <= 0, so use its absolute value
        double cos_a_plus_b = cos_plus * cos_minus - std::abs(sin_plus * sin_minus);

        // sharp if the sum is less than 90
        return cos_a_plus_b > 0;
}
}

template <std::size_t N>
void prune_facets_incident_to_sharp_ridges(
        const std::vector<Vector<N, double>>& points,
        const std::vector<DelaunayFacet<N>>& delaunay_facets,
        const std::vector<bool>& interior_vertices,
        std::vector<bool>* cocone_facets)
{
        namespace impl = prune_facets_implementation;

        ASSERT(!delaunay_facets.empty() && delaunay_facets.size() == cocone_facets->size());
        ASSERT(points.size() == interior_vertices.size());

        impl::RidgeMap<N> ridge_map;
        std::unordered_map<const DelaunayFacet<N>*, int> facets_ptr;
        for (unsigned i = 0; i < delaunay_facets.size(); ++i)
        {
                if ((*cocone_facets)[i])
                {
                        add_to_ridges(delaunay_facets[i], &ridge_map);
                        facets_ptr.emplace(&delaunay_facets[i], i);
                }
        }

        impl::RidgeSet<N> suspicious_ridges(ridge_map.size());
        for (const auto& e : ridge_map)
        {
                suspicious_ridges.insert(e.first);
        }

        while (!suspicious_ridges.empty())
        {
                impl::RidgeSet<N> tmp_ridges;

                for (const Ridge<N>& r : suspicious_ridges)
                {
                        auto ridge_iter = ridge_map.find(r);
                        if (ridge_iter == ridge_map.cend())
                        {
                                continue;
                        }

                        if (!impl::sharp_ridge(points, interior_vertices, ridge_iter->first, ridge_iter->second))
                        {
                                continue;
                        }

                        std::vector<const DelaunayFacet<N>*> facets_to_remove;
                        facets_to_remove.reserve(ridge_iter->second.size());

                        for (auto d = ridge_iter->second.cbegin(); d != ridge_iter->second.cend(); ++d)
                        {
                                add_to_ridges(*(d->facet()), d->point(), &tmp_ridges);
                                facets_to_remove.push_back(d->facet());

                                auto del = facets_ptr.find(d->facet());
                                ASSERT(del != facets_ptr.cend());
                                (*cocone_facets)[del->second] = false;
                        }

                        for (const DelaunayFacet<N>* facet : facets_to_remove)
                        {
                                remove_from_ridges(*facet, &ridge_map);
                        }
                }

                suspicious_ridges = std::move(tmp_ridges);
        }
}
}
