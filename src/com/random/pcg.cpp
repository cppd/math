/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "pcg.h"

#include "device.h"

namespace ns
{
namespace
{
void init(const std::array<std::uint32_t, 4>& data, std::uint64_t* const state, std::uint64_t* const increment)
{
        *state = data[0];
        *state |= static_cast<std::uint64_t>(data[1]) << 32;
        *increment = data[2];
        *increment |= static_cast<std::uint64_t>(data[3]) << 32;
        *increment |= 1;
}
}

PCG::PCG()
{
        std::array<std::uint32_t, 4> data;
        read_system_random(std::as_writable_bytes(std::span(data)));
        init(data, &state_, &increment_);
}

PCG::PCG(std::seed_seq& seed_seq)
{
        std::array<std::uint32_t, 4> data;
        seed_seq.generate(data.begin(), data.end());
        init(data, &state_, &increment_);
}

PCG::PCG(const result_type value)
{
        std::seed_seq seed_seq({value});
        std::array<std::uint32_t, 4> data;
        seed_seq.generate(data.begin(), data.end());
        init(data, &state_, &increment_);
}
}
