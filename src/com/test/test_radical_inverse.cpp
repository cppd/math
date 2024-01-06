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

#include <src/com/radical_inverse.h>
#include <src/com/type/limit.h>

namespace ns
{
namespace
{
template <typename Result, unsigned BASE, typename T>
constexpr bool compare(const T v, const T v1, const T v2)
{
        return static_cast<Result>(v1) / v2 == radical_inverse<BASE, Result>(v);
}

template <unsigned BASE, typename T>
constexpr bool compare(const T v, const T v1, const T v2)
{
        return compare<float, BASE>(v, v1, v2) && compare<double, BASE>(v, v1, v2)
               && compare<long double, BASE>(v, v1, v2);
}

static_assert(compare<2>(0, 0, 1));
static_assert(compare<2>(1, 1, 2));

static_assert(compare<3>(0, 0, 1));
static_assert(compare<3>(2, 2, 3));

static_assert(compare<4>(0, 0, 1));
static_assert(compare<4>(3, 3, 4));

static_assert(compare<5>(0, 0, 1));
static_assert(compare<5>(4, 4, 5));

static_assert(compare<2>(0b101011, 0b110101, 0b1000000));
static_assert(compare<5>(1 * (5 * 5) + 2 * (5) + 3, 3 * (5 * 5) + 2 * (5) + 1, 5 * 5 * 5));
static_assert(compare<8>(0'1020'3040, 0'0403'0201, 01'0000'0000));
static_assert(compare<10>(123, 321, 1000));
static_assert(compare<11>(1 * (11 * 11) + 2 * (11) + 3, 3 * (11 * 11) + 2 * (11) + 1, 11 * 11 * 11));
static_assert(compare<16>(0x123456789F, 0xF987654321, 0x1'00'0000'0000));

//

template <unsigned BASE, typename Result, typename T>
constexpr bool check_max()
{
        return 1 > radical_inverse<BASE, Result>(Limits<T>::max());
}

template <unsigned BASE, typename T>
constexpr bool check_max()
{
        return check_max<BASE, float, T>() && check_max<BASE, double, T>() && check_max<BASE, long double, T>();
}

template <unsigned BASE>
constexpr bool check_max()
{
        return check_max<BASE, int>() && check_max<BASE, unsigned>() && check_max<BASE, long long>()
               && check_max<BASE, unsigned long long>();
}

static_assert(check_max<2>());
static_assert(check_max<3>());
static_assert(check_max<4>());
static_assert(check_max<5>());
static_assert(check_max<7>());
static_assert(check_max<111>());
}
}
