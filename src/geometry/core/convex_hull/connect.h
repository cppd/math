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

#include "../ridge.h"

#include <tuple>
#include <unordered_map>

namespace ns::geometry::convex_hull
{
template <std::size_t N, typename Facet>
class Connect
{
        std::unordered_map<Ridge<N>, std::tuple<Facet*, unsigned>> search_map_;
        int expected_ridge_count_;
        int ridge_count_ = 0;

public:
        Connect(const int expected_ridge_count)
                : search_map_(expected_ridge_count), expected_ridge_count_(expected_ridge_count)
        {
        }

        ~Connect()
        {
                ASSERT(ridge_count_ == expected_ridge_count_);
                ASSERT(search_map_.empty());
        }

        void connect_facets(Facet* const facet, const int exclude_point)
        {
                const std::array<int, N>& vertices = facet->vertices();
                for (unsigned r = 0; r < N; ++r)
                {
                        if (vertices[r] == exclude_point)
                        {
                                // The horizon ridge. The facet is aleady
                                // connected to it when the facet was created.
                                continue;
                        }

                        Ridge<N> ridge(del_elem(vertices, r));

                        const auto search_iter = search_map_.find(ridge);
                        if (search_iter == search_map_.cend())
                        {
                                search_map_.emplace(std::move(ridge), std::make_tuple(facet, r));
                        }
                        else
                        {
                                Facet* const link_facet = std::get<0>(search_iter->second);
                                const unsigned link_r = std::get<1>(search_iter->second);

                                facet->set_link(r, link_facet);
                                link_facet->set_link(link_r, facet);

                                search_map_.erase(search_iter);

                                ++ridge_count_;
                        }
                }
        }
};
}
