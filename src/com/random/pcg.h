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

#include <array>
#include <bit>
#include <cstdint>
#include <random>

namespace ns
{
class PCG final
{
        std::uint64_t state_;
        std::uint64_t increment_;

        explicit PCG(const std::array<std::uint32_t, 4>& data);

public:
        using result_type = std::uint32_t;

        [[nodiscard]] static constexpr result_type min()
        {
                return 0;
        }

        [[nodiscard]] static constexpr result_type max()
        {
                return 0xffff'ffff;
        }

        PCG();

        explicit PCG(std::seed_seq& seed_seq);

        explicit PCG(result_type value);

        [[nodiscard]] result_type operator()()
        {
                constexpr std::uint64_t MULTIPLIER = 6'364136'223846'793005;

                const std::uint64_t x = state_;
                state_ = x * MULTIPLIER + increment_;
                const std::uint32_t s = ((x >> 18) ^ x) >> 27;
                const std::uint32_t r = x >> 59;
                return std::rotr(s, r);
                // return (s >> r) | (s << ((-r) & 31));
        }
};
}
