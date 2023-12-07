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

#include "pcg.h"

#include "device.h"

#include <array>
#include <cstdint>
#include <random>
#include <span>

namespace ns
{
namespace
{
std::array<std::uint32_t, 4> random_data()
{
        std::array<std::uint32_t, 4> res;
        read_system_random(std::as_writable_bytes(std::span(res)));
        return res;
}

std::array<std::uint32_t, 4> random_data(std::seed_seq& seed_seq)
{
        std::array<std::uint32_t, 4> res;
        seed_seq.generate(res.begin(), res.end());
        return res;
}

template <typename T>
        requires (std::is_integral_v<T>)
std::array<std::uint32_t, 4> random_data(const T value)
{
        std::seed_seq seed_seq({value});
        std::array<std::uint32_t, 4> res;
        seed_seq.generate(res.begin(), res.end());
        return res;
}

//

std::uint64_t state(const std::uint32_t v1, const std::uint32_t v2)
{
        std::uint64_t res = v1;
        res |= static_cast<std::uint64_t>(v2) << 32;
        return res;
}

std::uint64_t increment(const std::uint32_t v1, const std::uint32_t v2)
{
        std::uint64_t res = v1;
        res |= static_cast<std::uint64_t>(v2) << 32;
        res |= 1;
        return res;
}
}

PCG::PCG(const std::array<std::uint32_t, 4>& data)
        : state_(state(data[0], data[1])),
          increment_(increment(data[2], data[3]))
{
}

PCG::PCG()
        : PCG(random_data())
{
}

PCG::PCG(std::seed_seq& seed_seq)
        : PCG(random_data(seed_seq))
{
}

PCG::PCG(const result_type value)
        : PCG(random_data(value))
{
}
}
