/*
Copyright (C) 2017-2026 Topological Manifold

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
#include <src/geometry/core/ridge.h>

#include <array>
#include <cstddef>
#include <tuple>
#include <unordered_map>

namespace ns::geometry::core::convex_hull
{
template <std::size_t N, typename Facet>
class FacetConnector final
{
        std::unordered_map<Ridge<N>, std::tuple<Facet*, unsigned>> ridge_map_;
        std::size_t expected_ridge_count_;
        std::size_t ridge_count_ = 0;

public:
        explicit FacetConnector(const std::size_t expected_ridge_count)
                : ridge_map_(expected_ridge_count),
                  expected_ridge_count_(expected_ridge_count)
        {
        }

        ~FacetConnector()
        {
                ASSERT(ridge_count_ == expected_ridge_count_);
                ASSERT(ridge_map_.empty());
        }

        void connect(Facet* const facet, const int exclude_point)
        {
                const std::array<int, N>& vertices = facet->vertices();

                for (std::size_t r = 0; r < N; ++r)
                {
                        if (vertices[r] == exclude_point)
                        {
                                // The horizon ridge. The facet is aleady
                                // connected to it when the facet was created.
                                continue;
                        }

                        Ridge<N> ridge(del_elem(vertices, r));

                        const auto iter = ridge_map_.find(ridge);
                        if (iter == ridge_map_.cend())
                        {
                                ridge_map_.emplace(std::move(ridge), std::make_tuple(facet, r));
                                continue;
                        }

                        const auto [link_facet, link_r] = iter->second;

                        facet->set_link(r, link_facet);
                        link_facet->set_link(link_r, facet);

                        ridge_map_.erase(iter);

                        ++ridge_count_;
                }
        }
};
}
