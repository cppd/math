/*
Copyright (C) 2017-2021 Topological Manifold

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
#include <random>

namespace ns
{
class PCG final
{
        std::uint64_t state_;
        std::uint64_t increment_;

        void init(std::seed_seq& seed_seq)
        {
                std::array<std::uint32_t, 4> v;
                seed_seq.generate(v.begin(), v.end());
                state_ = v[0];
                state_ |= static_cast<std::uint64_t>(v[1]) << 32;
                increment_ = v[2];
                increment_ |= static_cast<std::uint64_t>(v[3]) << 32;
                increment_ |= 1;
        }

public:
        using result_type = std::uint32_t;

        static constexpr result_type min()
        {
                return 0;
        }

        static constexpr result_type max()
        {
                return 0xffff'ffff;
        }

        explicit PCG(std::seed_seq& seed_seq)
        {
                init(seed_seq);
        }

        explicit PCG(const result_type value)
        {
                std::seed_seq seed_seq({value});
                init(seed_seq);
        }

        result_type operator()()
        {
                static constexpr std::uint64_t MULTIPLIER = 6364136223846793005;

                const std::uint64_t x = state_;
                state_ = x * MULTIPLIER + increment_;
                const std::uint32_t s = ((x >> 18) ^ x) >> 27;
                const std::uint32_t r = x >> 59;
                return std::rotr(s, r);
                // return (s >> r) | (s << ((-r) & 31));
        }
};
}
