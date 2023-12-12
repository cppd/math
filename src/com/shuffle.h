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

#include "error.h"

#include <cstddef>
#include <random>
#include <tuple>

namespace ns
{
// Donald Knuth. The Art of Computer Programming. Second edition. Addison-Wesley, 1981.
// Volume 2. Seminumerical Algorithms. 3.4.2. Random Sampling and Shuffling.

template <typename RandomEngine, typename T>
void shuffle_dimension(RandomEngine&& engine, const std::size_t dimension, T* const data)
{
        ASSERT(dimension < std::tuple_size_v<typename T::value_type>);

        if (data->size() < 2)
        {
                return;
        }

        for (std::size_t i = data->size() - 1; i > 0; --i)
        {
                const std::size_t j = std::uniform_int_distribution<std::size_t>(0, i)(engine);
                std::swap((*data)[i][dimension], (*data)[j][dimension]);
        }
}

template <typename RandomEngine, typename... T>
void shuffle(RandomEngine&& engine, T* const... data)
{
        static_assert(sizeof...(T) >= 1);

        ASSERT((data && ...));

        const std::size_t size = std::get<0>(std::make_tuple(data->size()...));

        if constexpr (sizeof...(T) >= 2)
        {
                ASSERT(((data->size() == size) && ...));
        }

        if (size < 2)
        {
                return;
        }

        for (std::size_t i = size - 1; i > 0; --i)
        {
                const std::size_t j = std::uniform_int_distribution<std::size_t>(0, i)(engine);
                (std::swap((*data)[i], (*data)[j]), ...);
        }
}
}
