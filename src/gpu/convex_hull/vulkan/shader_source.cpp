/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "shader_source.h"

constexpr uint32_t prepare_comp[]{
#include "convex_hull_prepare.comp.spr"
};
constexpr uint32_t merge_comp[]{
#include "convex_hull_merge.comp.spr"
};
constexpr uint32_t filter_comp[]{
#include "convex_hull_filter.comp.spr"
};
constexpr uint32_t show_vert[]{
#include "convex_hull_show.vert.spr"
};
constexpr uint32_t show_frag[]{
#include "convex_hull_show.frag.spr"
};

namespace gpu_vulkan
{
Span<const uint32_t> convex_hull_prepare_comp()
{
        return prepare_comp;
}

Span<const uint32_t> convex_hull_merge_comp()
{
        return merge_comp;
}

Span<const uint32_t> convex_hull_filter_comp()
{
        return filter_comp;
}

Span<const uint32_t> convex_hull_show_frag()
{
        return show_frag;
}

Span<const uint32_t> convex_hull_show_vert()
{
        return show_vert;
}
}
