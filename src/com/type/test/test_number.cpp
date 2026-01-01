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

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/number.h>
#include <src/test/test.h>

#include <cmath>

namespace ns
{
namespace
{
template <typename T>
struct Check final
{
        static constexpr T NEGATIVE_EPSILON = T{1} - PREVIOUS_BEFORE_ONE<T>;
        static_assert(1 - NEGATIVE_EPSILON < 1);
        static_assert(1 - (NEGATIVE_EPSILON / 2) == 1);
};
template struct Check<float>;
template struct Check<double>;
template struct Check<long double>;

template <typename T>
void test_negative_epsilon()
{
        const T next = std::nextafter(T{1}, T{0});
        if (!(PREVIOUS_BEFORE_ONE<T> == next))
        {
                error("Next before one " + to_string(PREVIOUS_BEFORE_ONE<T>) + " is not equal to nextafter(1, 0) "
                      + to_string(next));
        }
}

void test()
{
        test_negative_epsilon<float>();
        test_negative_epsilon<double>();
        test_negative_epsilon<long double>();
}

TEST_SMALL("Type Numbers", test)
}
}
