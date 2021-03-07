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

#include "error.h"

#include <random>
#include <tuple>
#include <vector>

namespace ns
{
// Donald Knuth. The Art of Computer Programming. Second edition. Addison-Wesley, 1981.
// Volume 2. Seminumerical Algorithms. 3.4.2. Random Sampling and Shuffling.
template <typename T, typename RandomEngine>
void shuffle_dimension(RandomEngine& random_engine, std::size_t dimension, std::vector<T>* v)
{
        ASSERT(dimension < std::tuple_size_v<T>);

        if (v->size() < 2)
        {
                return;
        }

        for (std::size_t i = v->size() - 1; i > 0; --i)
        {
                std::size_t j = std::uniform_int_distribution<std::size_t>(0, i)(random_engine);
                std::swap((*v)[i][dimension], (*v)[j][dimension]);
        }
}
}
