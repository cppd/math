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

//#include "container.h"

#include <array>
#include <functional>
//#include <string_view>
//#include <type_traits>

namespace ns
{
#if 0
constexpr uint32_t jenkins_one_at_a_time_hash(const char* key, int len)
{
        uint32_t hash = 0;
        for (int i = 0; i < len; ++i)
        {
                hash += key[i];
                hash += (hash << 10);
                hash ^= (hash >> 6);
        }
        hash += (hash << 3);
        hash ^= (hash >> 11);
        hash += (hash << 15);
        return hash;
}
#endif

#if 0
template <typename T, std::size_t N>
std::size_t hash_as_string(const std::array<T, N>& v)
{
        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);

        const char* s = reinterpret_cast<const char*>(v.data());
        std::size_t size = storage_size(v);

        // return jenkins_one_at_a_time_hash(s, size);
        return std::hash<std::string_view>()(std::string_view(s, size));
}
#endif

template <typename T, std::size_t N>
std::size_t array_hash(const std::array<T, N>& v)
{
        std::hash<T> hasher;
        std::size_t seed = hasher(v[0]);
        for (unsigned i = 1; i < N; ++i)
        {
                seed ^= hasher(v[i]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
}

template <typename T, typename... Ts>
std::size_t pack_hash(const T& v, const Ts&... vs)
{
        static_assert((std::is_same_v<T, Ts> && ...));
        std::hash<T> hasher;
        std::size_t seed = hasher(v);
        ((seed ^= hasher(vs) + 0x9e3779b9 + (seed << 6) + (seed >> 2)), ...);
        return seed;
}
}
