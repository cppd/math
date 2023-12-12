/*
Copyright (C) 2017-2023 Topological Manifold

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
#include <src/numerical/vector.h>
#include <src/progress/progress.h>

#include <array>
#include <cstddef>
#include <vector>

namespace ns::geometry::core
{
template <std::size_t N>
class ConvexHullSimplex final
{
        std::array<int, N> indices_;
        Vector<N, double> ortho_;

public:
        ConvexHullSimplex(const std::array<int, N>& indices, const Vector<N, double>& ortho)
                : indices_(indices),
                  ortho_(ortho)
        {
        }

        [[nodiscard]] const std::array<int, N>& vertices() const
        {
                return indices_;
        }

        [[nodiscard]] const Vector<N, double>& ortho() const
        {
                return ortho_;
        }
};

template <std::size_t N>
class DelaunaySimplex final
{
        std::array<int, N + 1> indices_;
        std::array<Vector<N, double>, N + 1> orthos_;

public:
        DelaunaySimplex(const std::array<int, N + 1>& indices, const std::array<Vector<N, double>, N + 1>& orthos)
                : indices_(indices),
                  orthos_(orthos)
        {
        }

        [[nodiscard]] const std::array<int, N + 1>& vertices() const
        {
                return indices_;
        }

        [[nodiscard]] const Vector<N, double>& ortho(const unsigned index) const
        {
                ASSERT(index < orthos_.size());
                return orthos_[index];
        }
};

template <std::size_t N>
struct DelaunayData final
{
        std::vector<Vector<N, double>> points;
        std::vector<DelaunaySimplex<N>> simplices;
};

template <std::size_t N>
DelaunayData<N> compute_delaunay(
        const std::vector<Vector<N, float>>& points,
        progress::Ratio* progress,
        bool write_log);

template <std::size_t N>
std::vector<ConvexHullSimplex<N>> compute_convex_hull(
        const std::vector<Vector<N, float>>& points,
        progress::Ratio* progress,
        bool write_log);
}
