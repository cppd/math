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

#include <src/filter/filters/com/variance.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>

namespace ns::filter::filters::speed::filter_2_conv
{
template <std::size_t N, typename T>
[[nodiscard]] numerical::Vector<N / 3, T> position(const numerical::Vector<N, T>& x)
{
        return numerical::slice<0, 3>(x);
}

template <std::size_t N, typename T>
[[nodiscard]] numerical::Matrix<N / 3, N / 3, T> position_p(const numerical::Matrix<N, N, T>& p)
{
        return numerical::slice<0, 3>(p);
}

template <std::size_t N, typename T>
[[nodiscard]] numerical::Vector<N / 3, T> velocity(const numerical::Vector<N, T>& x)
{
        return numerical::slice<1, 3>(x);
}

template <std::size_t N, typename T>
[[nodiscard]] numerical::Matrix<N / 3, N / 3, T> velocity_p(const numerical::Matrix<N, N, T>& p)
{
        return numerical::slice<1, 3>(p);
}

template <std::size_t N, typename T>
[[nodiscard]] T speed(const numerical::Vector<N, T>& x)
{
        return velocity(x).norm();
}

template <std::size_t N, typename T>
[[nodiscard]] T speed_p(const numerical::Vector<N, T>& x, const numerical::Matrix<N, N, T>& p)
{
        return com::compute_speed_variance(velocity(x), velocity_p(p));
}
}
