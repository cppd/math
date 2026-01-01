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

#include "compare.h"

namespace ns::geometry::shapes::test
{
static_assert(compare(1, 1.1, 1.1));
static_assert(compare(1000, 10000.100000001, 10000.100000002));
static_assert(!compare(1, 10000.100000001, 10000.100000002));
static_assert(!compare(1, 10000.100000002, 10000.100000001));
}
