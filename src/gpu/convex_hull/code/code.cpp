/*
Copyright (C) 2017-2023 Topological Manifold

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

namespace ns::gpu::convex_hull
{
std::vector<std::uint32_t> code_prepare_comp()
{
        static constexpr std::uint32_t CODE[] = {
#include "convex_hull_prepare.comp.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_merge_comp()
{
        static constexpr std::uint32_t CODE[] = {
#include "convex_hull_merge.comp.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_filter_comp()
{
        static constexpr std::uint32_t CODE[] = {
#include "convex_hull_filter.comp.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_view_frag()
{
        static constexpr std::uint32_t CODE[] = {
#include "convex_hull_view.frag.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_view_vert()
{
        static constexpr std::uint32_t CODE[] = {
#include "convex_hull_view.vert.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}
}
