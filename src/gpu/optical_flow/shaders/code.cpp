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

#include "code.h"

namespace gpu::optical_flow
{
namespace
{
constexpr uint32_t sobel_comp[]{
#include "optical_flow_sobel.comp.spr"
};
constexpr uint32_t flow_comp[]{
#include "optical_flow_flow.comp.spr"
};
constexpr uint32_t downsample_comp[]{
#include "optical_flow_downsample.comp.spr"
};
constexpr uint32_t grayscale_comp[]{
#include "optical_flow_grayscale.comp.spr"
};
constexpr uint32_t view_vert[]{
#include "optical_flow_view.vert.spr"
};
constexpr uint32_t view_frag[]{
#include "optical_flow_view.frag.spr"
};
constexpr uint32_t view_debug_vert[]{
#include "optical_flow_view_debug.vert.spr"
};
constexpr uint32_t view_debug_frag[]{
#include "optical_flow_view_debug.frag.spr"
};
}

Span<const uint32_t> code_sobel_comp()
{
        return sobel_comp;
}

Span<const uint32_t> code_flow_comp()
{
        return flow_comp;
}

Span<const uint32_t> code_downsample_comp()
{
        return downsample_comp;
}

Span<const uint32_t> code_grayscale_comp()
{
        return grayscale_comp;
}

Span<const uint32_t> code_view_vert()
{
        return view_vert;
}

Span<const uint32_t> code_view_frag()
{
        return view_frag;
}

Span<const uint32_t> code_view_debug_vert()
{
        return view_debug_vert;
}

Span<const uint32_t> code_view_debug_frag()
{
        return view_debug_frag;
}
}
