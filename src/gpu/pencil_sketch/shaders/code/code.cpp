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
std::vector<uint32_t> code_compute_comp()
{
        return {
#include "pencil_sketch_compute.comp.spr"
        };
}

std::vector<uint32_t> code_view_vert()
{
        return {
#include "pencil_sketch_view.vert.spr"
        };
}

std::vector<uint32_t> code_view_frag()
{
        return {
#include "pencil_sketch_view.frag.spr"
        };
}
}
