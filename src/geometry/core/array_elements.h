/*
Copyright (C) 2017 Topological Manifold

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

#include "com/error.h"

#include <array>

// template <typename T, size_t N>
// std::array<T, N + 1> add_elem(const std::array<T, N>& a, const T& v)
//{
//        static_assert(N >= 1);

//        std::array<T, N + 1> res;
//        for (unsigned i = 0; i < N; ++i)
//        {
//                res[i] = a[i];
//        }
//        res[N] = v;

//        return res;
//}

template <typename T, size_t N>
constexpr std::array<T, N> set_elem(const std::array<T, N>& a, unsigned pos, const T& v)
{
        static_assert(N >= 1);
        ASSERT(pos < N);

        std::array<T, N> res(a);
        res[pos] = v;

        return res;
}

template <typename T, size_t N>
std::array<T, N - 1> del_elem(const std::array<T, N>& a, unsigned pos)
{
        static_assert(N > 1);
        ASSERT(pos < N);

        std::array<T, N - 1> res;
        for (unsigned i = 0; i < pos; ++i)
        {
                res[i] = a[i];
        }
        for (unsigned i = pos + 1; i < N; ++i)
        {
                res[i - 1] = a[i];
        }

        return res;
}

template <typename T>
constexpr std::array<T, 1> del_elem(const std::array<T, 2>& a, unsigned pos)
{
        switch (pos)
        {
        case 0:
                return {{a[1]}};
        case 1:
                return {{a[0]}};
        }
        error("pos >= 2");
}

template <typename T>
constexpr std::array<T, 2> del_elem(const std::array<T, 3>& a, unsigned pos)
{
        switch (pos)
        {
        case 0:
                return {{a[1], a[2]}};
        case 1:
                return {{a[0], a[2]}};
        case 2:
                return {{a[0], a[1]}};
        }
        error("pos >= 3");
}

template <typename T>
constexpr std::array<T, 3> del_elem(const std::array<T, 4>& a, unsigned pos)
{
        switch (pos)
        {
        case 0:
                return {{a[1], a[2], a[3]}};
        case 1:
                return {{a[0], a[2], a[3]}};
        case 2:
                return {{a[0], a[1], a[3]}};
        case 3:
                return {{a[0], a[1], a[2]}};
        }
        error("pos >= 4");
}
