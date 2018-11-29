/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "groups.h"

static_assert(group_count(11, 4) == 3);
static_assert(group_count(12, 4) == 3);
static_assert(group_count(13, 4) == 4);
static_assert(group_count(14, 4) == 4);
static_assert(group_count(1, 100) == 1);
static_assert(group_count(100, 1) == 100);
static_assert(group_count(11, 17, vec2i(4, 5)) == vec2i(3, 4));
static_assert(group_count(11, 17, 19, vec3i(4, 5, 3)) == vec3i(3, 4, 7));
