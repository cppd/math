/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "system.h"

#include <type_traits>

// T == std::some_random_number_engine
template <typename T>
class RandomEngineWithSeed
{
        T m_random_engine;

        static typename T::result_type random_seed()
        {
                static_assert(std::is_integral_v<result_type> && std::is_unsigned_v<result_type>);

                result_type v;
                read_system_random(&v, sizeof(v));
                return v;
        }

public:
        using result_type = typename T::result_type;

        RandomEngineWithSeed() : m_random_engine(random_seed())
        {
        }

        operator T&()
        {
                return m_random_engine;
        }

        typename T::result_type operator()()
        {
                return m_random_engine();
        }

        static constexpr typename T::result_type min()
        {
                return T::min();
        }

        static constexpr typename T::result_type max()
        {
                return T::max();
        }
};
