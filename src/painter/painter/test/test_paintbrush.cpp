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

#include "../paintbrush.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/test/test.h>

namespace ns::painter
{
namespace
{
std::string str(const std::optional<std::array<int_least16_t, 2>>& v)
{
        if (v)
        {
                return to_string(*v);
        }
        return "nullopt";
}

void check_impl(
        const std::optional<std::array<int_least16_t, 2>>& p,
        const std::optional<std::array<int_least16_t, 2>>& c)
{
        if (p != c)
        {
                error("Error paintbrush pixel " + str(p) + ", expected " + str(c));
        }
}

void check(const std::optional<std::array<int_least16_t, 2>>& p, const std::array<int_least16_t, 2>& c)
{
        check_impl(p, c);
}

void check(const std::optional<std::array<int_least16_t, 2>>& p, const std::nullopt_t& c)
{
        check_impl(p, c);
}

void test()
{
        Paintbrush<2> paintbrush({4, 4}, 3);
        for (int i = 0; i < 2; ++i)
        {
                check(paintbrush.next_pixel(), {0, 3});
                check(paintbrush.next_pixel(), {0, 2});
                check(paintbrush.next_pixel(), {0, 1});
                check(paintbrush.next_pixel(), {1, 3});
                check(paintbrush.next_pixel(), {1, 2});
                check(paintbrush.next_pixel(), {1, 1});
                check(paintbrush.next_pixel(), {2, 3});
                check(paintbrush.next_pixel(), {2, 2});
                check(paintbrush.next_pixel(), {2, 1});
                check(paintbrush.next_pixel(), {3, 3});
                check(paintbrush.next_pixel(), {3, 2});
                check(paintbrush.next_pixel(), {3, 1});
                check(paintbrush.next_pixel(), {0, 0});
                check(paintbrush.next_pixel(), {1, 0});
                check(paintbrush.next_pixel(), {2, 0});
                check(paintbrush.next_pixel(), {3, 0});
                check(paintbrush.next_pixel(), std::nullopt);
                paintbrush.reset();
        }
}

TEST_SMALL("Paintbrush", test)
}
}
