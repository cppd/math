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

#include "code.h"

#include <cstdint>
#include <iterator>
#include <vector>

namespace ns::gpu::pencil_sketch
{
std::vector<std::uint32_t> code_compute_comp()
{
        static constexpr std::uint32_t CODE[] = {
#include "pencil_sketch_compute.comp.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_view_vert()
{
        static constexpr std::uint32_t CODE[] = {
#include "pencil_sketch_view.vert.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_view_frag()
{
        static constexpr std::uint32_t CODE[] = {
#include "pencil_sketch_view.frag.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}
}
