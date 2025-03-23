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

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>

namespace ns::filter::filters::position::filter_0_conv
{
template <std::size_t N, typename T>
[[nodiscard]] numerical::Vector<N, T> position(const numerical::Vector<N, T>& x)
{
        return x;
}

template <std::size_t N, typename T>
[[nodiscard]] numerical::Matrix<N, N, T> position_p(const numerical::Matrix<N, N, T>& p)
{
        return p;
}
}
