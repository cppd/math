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

#include "../union_find.h"

#include <src/com/error.h>
#include <src/test/test.h>

namespace ns
{
namespace
{
void check_true(const bool v)
{
        if (!v)
        {
                error("No connection found");
        }
}

void check_false(const bool v)
{
        if (v)
        {
                error("Unexpected connection");
        }
}

void test()
{
        UnionFind<int> uf(10);

        check_true(uf.add_connection(0, 1));
        check_true(uf.add_connection(1, 2));
        check_true(uf.add_connection(2, 3));
        check_true(uf.add_connection(4, 5));
        check_true(uf.add_connection(6, 7));
        check_true(uf.add_connection(2, 7));

        check_false(uf.add_connection(1, 3));
        check_false(uf.add_connection(0, 6));
        check_false(uf.add_connection(1, 7));
}

TEST_SMALL("Union-Find", test)
}
}
