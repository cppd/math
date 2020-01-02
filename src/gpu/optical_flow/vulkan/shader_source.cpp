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

#include "shader_source.h"

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
constexpr uint32_t show_vert[]{
#include "optical_flow_show.vert.spr"
};
constexpr uint32_t show_frag[]{
#include "optical_flow_show.frag.spr"
};
constexpr uint32_t show_debug_vert[]{
#include "optical_flow_show_debug.vert.spr"
};
constexpr uint32_t show_debug_frag[]{
#include "optical_flow_show_debug.frag.spr"
};

namespace gpu_vulkan
{
Span<const uint32_t> optical_flow_sobel_comp()
{
        return sobel_comp;
}

Span<const uint32_t> optical_flow_flow_comp()
{
        return flow_comp;
}

Span<const uint32_t> optical_flow_downsample_comp()
{
        return downsample_comp;
}

Span<const uint32_t> optical_flow_grayscale_comp()
{
        return grayscale_comp;
}

Span<const uint32_t> optical_flow_show_vert()
{
        return show_vert;
}

Span<const uint32_t> optical_flow_show_frag()
{
        return show_frag;
}

Span<const uint32_t> optical_flow_show_debug_vert()
{
        return show_debug_vert;
}

Span<const uint32_t> optical_flow_show_debug_frag()
{
        return show_debug_frag;
}
}
