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

#pragma once

#include <src/com/error.h>
#include <src/numerical/vec.h>
#include <src/progress/progress.h>

#include <array>
#include <vector>

namespace ns::geometry
{
template <std::size_t N>
class ConvexHullFacet final
{
        const std::array<int, N> indices_;
        const Vector<N, double> ortho_;

public:
        ConvexHullFacet(const std::array<int, N>& indices, const Vector<N, double>& ortho)
                : indices_(indices), ortho_(ortho)
        {
        }
        const std::array<int, N>& vertices() const
        {
                return indices_;
        }
        const Vector<N, double>& ortho() const
        {
                return ortho_;
        }
};

template <std::size_t N>
class DelaunaySimplex final
{
        const std::array<int, N + 1> indices_;
        const std::array<Vector<N, double>, N + 1> orthos_;

public:
        DelaunaySimplex(const std::array<int, N + 1>& indices, const std::array<Vector<N, double>, N + 1>& orthos)
                : indices_(indices), orthos_(orthos)
        {
        }
        const std::array<int, N + 1>& vertices() const
        {
                return indices_;
        }
        const Vector<N, double>& ortho(unsigned i) const
        {
                ASSERT(i < orthos_.size());
                return orthos_[i];
        }
};

template <std::size_t N>
void compute_delaunay(
        const std::vector<Vector<N, float>>& source_points,
        std::vector<Vector<N, double>>* points,
        std::vector<DelaunaySimplex<N>>* simplices,
        ProgressRatio* progress,
        bool write_log);

template <std::size_t N>
void compute_convex_hull(
        const std::vector<Vector<N, float>>& source_points,
        std::vector<ConvexHullFacet<N>>* ch_facets,
        ProgressRatio* progress,
        bool write_log);
}
