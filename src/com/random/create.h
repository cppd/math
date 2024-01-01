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

#pragma once

#include "device.h"

#include <array>
#include <cstdint>
#include <random>
#include <span>

namespace ns
{
template <typename T>
T create_engine()
{
        std::array<std::uint_least32_t, 16> data;
        read_system_random(std::as_writable_bytes(std::span(data)));
        std::seed_seq seed_seq(data.cbegin(), data.cend());
        return T(seed_seq);
}
}
