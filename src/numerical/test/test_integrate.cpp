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

#include <src/com/math.h>
#include <src/numerical/integrate.h>

namespace ns::numerical
{
namespace
{
template <typename T>
constexpr bool equal(const T& a, const T& b, const T& precision)
{
        return absolute(a - b) <= precision;
}

template <typename T>
constexpr T f(const T& x)
{
        return x * x * x;
}

template <typename T>
constexpr bool test(const T& p_100, const T& p_1000, const T& p_10000)
{
        return equal(integrate<T>(f<T>, 1, 2, 100), T{15} / 4, p_100)
               && equal(integrate<T>(f<T>, -2, -1, 100), -T{15} / 4, p_100)
               && equal(integrate<T>(f<T>, 1, 2, 1000), T{15} / 4, p_1000)
               && equal(integrate<T>(f<T>, -2, -1, 1000), -T{15} / 4, p_1000)
               && equal(integrate<T>(f<T>, 1, 2, 10000), T{15} / 4, p_10000)
               && equal(integrate<T>(f<T>, -2, -1, 10000), -T{15} / 4, p_10000);
}

static_assert(test<float>(1e-4, 1e-5, 1e-5));
static_assert(test<double>(1e-4, 1e-6, 1e-8));
static_assert(test<long double>(1e-4, 1e-6, 1e-8));
static_assert(test<__float128>(1e-4, 1e-6, 1e-8));
}
}
