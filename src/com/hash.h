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

#include <cstddef>
#include <functional>

// #include <string_view>

namespace ns
{
template <typename T, typename... Ts>
std::size_t compute_hash(const T& v, const Ts&... vs)
{
        const auto combine = [](std::size_t& seed, const std::size_t hash)
        {
                seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        };

        static constexpr bool HAS_HASH = requires { std::hash<T>{}(v); };

        if constexpr (HAS_HASH)
        {
                static_assert((std::is_same_v<T, Ts> && ...));

                const std::hash<T> hasher;
                std::size_t seed = hasher(v);
                (combine(seed, hasher(vs)), ...);
                return seed;
        }
        // else if constexpr (std::has_unique_object_representations_v<T>)
        // {
        //         static_assert((std::is_same_v<T, Ts> && ...));
        //
        //         std::hash<std::string_view> hasher;
        //         std::size_t seed = hasher({reinterpret_cast<const char*>(&v), sizeof(v)});
        //         (combine(seed, hasher({reinterpret_cast<const char*>(&vs), sizeof(vs)})), ...);
        //         return seed;
        // }
        else
        {
                static_assert(sizeof...(Ts) == 0);

                static constexpr std::size_t N = std::tuple_size_v<T>;
                static_assert(N >= 1);
                const std::hash<typename T::value_type> hasher;
                std::size_t seed = hasher(v[0]);
                for (std::size_t i = 1; i < N; ++i)
                {
                        combine(seed, hasher(v[i]));
                }
                return seed;
        }
}
}
