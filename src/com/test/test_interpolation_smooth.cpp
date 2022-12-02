/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "../interpolation_smooth.h"

#include <src/com/type/limit.h>

namespace ns
{
namespace
{
template <typename T, Smooth SMOOTH>
struct Check1
{
        static constexpr T MAX = Limits<T>::max() / 2;
        static_assert(interpolation<SMOOTH, T>(1, MAX, T{0}) == 1);
        static_assert(interpolation<SMOOTH, T>(1, MAX, T{1}) == MAX);
        static_assert(interpolation<SMOOTH, T>(MAX, 1, T{0}) == MAX);
        static_assert(interpolation<SMOOTH, T>(MAX, 1, T{1}) == 1);
        static_assert(interpolation<SMOOTH, T>(1, MAX, T{1} / 2) == MAX / 2);
        static_assert(interpolation<SMOOTH, T>(MAX, 1, T{1} / 2) == MAX / 2);
};

template <typename T>
struct Check2 final : Check1<T, Smooth::N_1>, Check1<T, Smooth::N_2>, Check1<T, Smooth::N_3>
{
};

template struct Check2<float>;
template struct Check2<double>;
template struct Check2<long double>;
}
}
