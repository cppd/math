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

#include <cstddef>
#include <vector>

namespace ns::geometry::core::convex_hull
{
template <typename T>
class FacetStorage final
{
        // std::vector is faster than std::forward_list, std::list, std::set, std::unordered_set
        std::vector<const T*> data_;

public:
        void insert(const T* const facet)
        {
                data_.push_back(facet);
        }

        void erase(const T* const facet)
        {
                for (auto& v : data_)
                {
                        if (v != facet)
                        {
                                continue;
                        }
                        v = data_.back();
                        data_.pop_back();
                        return;
                }
                error("Facet not found in facet storage");
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
                return data_.size();
        }

        void clear()
        {
                data_.clear();
        }

        [[nodiscard]] auto begin() const noexcept
        {
                return data_.cbegin();
        }

        [[nodiscard]] auto end() const noexcept
        {
                return data_.cend();
        }
};
}
