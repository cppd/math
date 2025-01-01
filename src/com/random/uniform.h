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

#pragma once

#include <random>

namespace ns
{
template <typename RandomEngine, typename T>
[[nodiscard]] T uniform_distribution(RandomEngine& engine, std::uniform_real_distribution<T>& urd)
{
        // std::uniform_real_distribution may return b()
        T res;
        do
        {
                res = urd(engine);
        } while (!(res < urd.b()));
        return res;
}
}
