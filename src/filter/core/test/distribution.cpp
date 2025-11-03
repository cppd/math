/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "distribution.h"

#include <src/com/error.h>
#include <src/com/print.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <flat_map>
#include <string>
#include <unordered_map>
#include <vector>

namespace ns::filter::core::test
{
namespace
{
std::string distribution_to_string(const std::unordered_map<int, unsigned>& distribution)
{
        std::string res;
        for (const auto [k, v] : std::flat_map{distribution.cbegin(), distribution.cend()})
        {
                if (!res.empty())
                {
                        res += '\n';
                }
                res += to_string(k) + ":" + to_string(v);
        }
        return res;
}

void check_min_max(
        const std::unordered_map<int, unsigned>& distribution,
        const std::vector<unsigned>& expected_distribution)
{
        ASSERT(!distribution.empty());
        ASSERT(!expected_distribution.empty());

        const auto [min, max] = std::ranges::minmax_element(
                distribution,
                [](const auto& a, const auto& b)
                {
                        return a.first < b.first;
                });

        const int size = expected_distribution.size();

        if (!(std::abs(min->first) < size && std::abs(max->first) < size))
        {
                error("Filter distribution min max error\n" + distribution_to_string(distribution));
        }
}

auto map_value(const auto& map, const auto& value)
{
        const auto iter = map.find(value);
        return (iter != map.cend()) ? iter->second : 0;
}
}

template <typename T>
void Distribution<T>::add(const T difference, const T stddev)
{
        ++distribution_[static_cast<int>(difference / stddev)];
}

template <typename T>
void Distribution<T>::check(const std::vector<unsigned>& expected_distribution) const
{
        if (distribution_.empty())
        {
                error("Filter distribution is empty");
        }

        if (expected_distribution.empty())
        {
                error("Filter expected distribution is empty");
        }

        check_min_max(distribution_, expected_distribution);

        if (!(map_value(distribution_, 0) >= expected_distribution[0]))
        {
                error("Filter distribution zero error\n" + distribution_to_string(distribution_));
        }

        for (std::size_t i = 1; i < expected_distribution.size(); ++i)
        {
                const int index = i;
                const auto expected = expected_distribution[index];
                if (!(map_value(distribution_, index) <= expected && map_value(distribution_, -index) <= expected))
                {
                        error("Filter distribution compare error\n" + distribution_to_string(distribution_));
                }
        }
}

#define INSTANTIATION(T) template class Distribution<T>;

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
