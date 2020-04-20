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

namespace gpu::pencil_sketch
{
namespace
{
constexpr uint32_t compute_comp[]{
#include "pencil_sketch_compute.comp.spr"
};
constexpr uint32_t view_vert[]{
#include "pencil_sketch_view.vert.spr"
};
constexpr uint32_t view_frag[]{
#include "pencil_sketch_view.frag.spr"
};
}

Span<const uint32_t> code_compute_comp()
{
        return compute_comp;
}

Span<const uint32_t> code_view_vert()
{
        return view_vert;
}

Span<const uint32_t> code_view_frag()
{
        return view_frag;
}
}
