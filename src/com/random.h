/*
Copyright (C) 2017 Topological Manifold

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
#ifndef RANDOM_H
#define RANDOM_H

#include <type_traits>

void read_system_random(void* p, unsigned count);

// T == std::some_random_number_engine
template <typename T>
typename T::result_type get_random_seed()
{
        using result_type = typename T::result_type;

        static_assert(std::is_integral<result_type>::value && std::is_unsigned<result_type>::value);

        result_type v;
        read_system_random(&v, sizeof(v));
        return v;
}

#endif
