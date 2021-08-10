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

#include "../gauss.h"

namespace ns::numerical
{
namespace
{
// clang-format off
template <typename T>
constexpr std::array MATRIX = std::to_array<Vector<4, T>>
({
{2, 2, 3, 4},
{5, 12, 7, 8},
{9, 10, 22, 12},
{13, 14, 15, 32}
});
template <typename T>
constexpr std::array INVERSE = std::to_array<Vector<4, T>>
({
{T(99) / 10, T(1) / 10, T(-7) / 10, T(-1)},
{T(-61) / 50, T(3) / 25, T(3) / 50, T(1) / 10},
{T(-107) / 50, T(-3) / 50, T(11) / 50, T(1) / 5},
{T(-497) / 200, T(-13) / 200, T(31) / 200, T(3) / 10}
});
template <typename T>
constexpr std::array IDENTITY = std::to_array<Vector<4, T>>
({
{1, 0, 0, 0},
{0, 1, 0, 0},
{0, 0, 1, 0},
{0, 0, 0, 1}
});
// clang-format on
template <typename T>
constexpr Vector<4, T> ROW{1, 2, 3, 4};
template <typename T>
constexpr Vector<4, T> SOLVED{T(4), T(-2) / 5, T(-4) / 5, T(-19) / 20};

template <typename T>
constexpr T absolute(const T& v)
{
        return v < 0 ? -v : v;
}

template <typename T>
constexpr bool equal(const T& a, const T& b, const T& precision)
{
        if (a == b)
        {
                return true;
        }
        T rel = absolute(a - b) / std::max(absolute(a), absolute(b));
        return (rel < precision);
}

template <std::size_t N, typename T>
constexpr bool equal(const Vector<N, T>& a, const Vector<N, T>& b, const T& precision)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!equal(a[i], b[i], precision))
                {
                        return false;
                }
        }
        return true;
}

template <std::size_t N, typename T>
constexpr bool equal(const std::array<Vector<N, T>, N>& a, const std::array<Vector<N, T>, N>& b, const T& precision)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!equal(a[i], b[i], precision))
                {
                        return false;
                }
        }
        return true;
}

template <typename T>
constexpr bool test_solve(const T& precision)
{
        if (!equal(INVERSE<T>, solve_gauss(MATRIX<T>, IDENTITY<T>), precision))
        {
                return false;
        }
        if (!equal(SOLVED<T>, solve_gauss(MATRIX<T>, ROW<T>), precision))
        {
                return false;
        }
        return true;
}

static_assert(test_solve<float>(6e-7));
static_assert(test_solve<double>(2e-15));
static_assert(test_solve<long double>(7e-19));
static_assert(test_solve<__float128>(2e-33));
}
}
