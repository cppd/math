/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <cmath>

namespace ns
{
template <typename T>
T geometric_progression_sum(const T& ratio, const T& n)
{
        return (std::pow(ratio, n) - 1) / (ratio - 1);
}

template <typename T>
T geometric_progression_n(const T& ratio, const T& sum)
{
        return std::log(sum * (ratio - 1) + 1) / std::log(ratio);
}
}
