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

namespace ns::gpu::renderer::test
{
std::vector<std::uint32_t> code_ray_closest_hit_rchit()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_ray_closest_hit.rchit.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_ray_generation_rgen()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_ray_generation.rgen.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_ray_miss_rmiss()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_ray_miss.rmiss.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}

std::vector<std::uint32_t> code_ray_query_comp()
{
        static constexpr std::uint32_t CODE[] = {
#include "renderer_ray_query.comp.spr"
        };
        return {std::cbegin(CODE), std::cend(CODE)};
}
}
