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

#include <src/numerical/region.h>
#include <src/numerical/vector.h>

namespace ns
{
namespace
{
constexpr bool test_inside(const Region<2, int>& r, const Vector<2, int>& v)
{
        return r.is_inside(v) && r.is_inside(v[0], v[1]);
}

static_assert(test_inside(Region<2, int>({-1, 2}, {3, 4}), {-1, 2}));
static_assert(test_inside(Region<2, int>({-1, 2}, {3, 4}), {1, 2}));
static_assert(test_inside(Region<2, int>({-1, 2}, {3, 4}), {-1, 3}));
static_assert(test_inside(Region<2, int>({-1, 2}, {3, 4}), {1, 3}));

static_assert(test_inside(Region<2, int>({1, -2}, {3, 4}), {1, -2}));
static_assert(test_inside(Region<2, int>({1, -2}, {3, 4}), {3, -2}));
static_assert(test_inside(Region<2, int>({1, -2}, {3, 4}), {1, -1}));
static_assert(test_inside(Region<2, int>({1, -2}, {3, 4}), {3, -1}));

static_assert(!test_inside(Region<2, int>({-1, 2}, {3, 4}), {2, 3}));
static_assert(!test_inside(Region<2, int>({-1, 2}, {3, 4}), {1, 6}));
static_assert(!test_inside(Region<2, int>({-1, 2}, {3, 4}), {2, 6}));
static_assert(!test_inside(Region<2, int>({-1, 2}, {3, 4}), {10, 10}));
static_assert(!test_inside(Region<2, int>({-1, 2}, {3, 4}), {-10, -10}));

static_assert(!test_inside(Region<2, int>({1, -2}, {3, 4}), {4, -1}));
static_assert(!test_inside(Region<2, int>({1, -2}, {3, 4}), {3, 2}));
static_assert(!test_inside(Region<2, int>({1, -2}, {3, 4}), {4, 2}));
static_assert(!test_inside(Region<2, int>({1, -2}, {3, 4}), {12, 6}));
static_assert(!test_inside(Region<2, int>({1, -2}, {3, 4}), {-8, -14}));

static_assert(Region<2, int>({0, 0}, {3, 4}).is_positive());
static_assert(Region<2, int>({1, 2}, {3, 4}).is_positive());

static_assert(!Region<2, int>({0, 0}, {0, 0}).is_positive());
static_assert(!Region<2, int>({1, 2}, {0, 0}).is_positive());
static_assert(!Region<2, int>({-1, -2}, {0, 0}).is_positive());
static_assert(!Region<2, int>({1, -2}, {3, 4}).is_positive());
static_assert(!Region<2, int>({-1, 2}, {3, 4}).is_positive());
static_assert(!Region<2, int>({-1, -2}, {3, 4}).is_positive());
static_assert(!Region<2, int>({1, 2}, {3, -4}).is_positive());
static_assert(!Region<2, int>({1, 2}, {-3, 4}).is_positive());
static_assert(!Region<2, int>({1, 2}, {-3, -4}).is_positive());
static_assert(!Region<2, int>({-1, -2}, {-3, -4}).is_positive());
}
}
