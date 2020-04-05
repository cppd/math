/*
Copyright (C) 2017-2020 Topological Manifold

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

#include <src/com/span.h>

namespace gpu
{
Span<const uint32_t> convex_hull_prepare_comp();
Span<const uint32_t> convex_hull_merge_comp();
Span<const uint32_t> convex_hull_filter_comp();
Span<const uint32_t> convex_hull_show_frag();
Span<const uint32_t> convex_hull_show_vert();
}
