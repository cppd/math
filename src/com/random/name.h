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

#include <random>
#include <type_traits>

namespace ns
{
template <typename T>
constexpr const char* random_engine_name() requires std::is_same_v<std::remove_cv_t<T>, std::knuth_b>
{
        return "std::knuth_b";
}

template <typename T>
constexpr const char* random_engine_name() requires std::is_same_v<std::remove_cv_t<T>, std::minstd_rand>
{
        return "std::minstd_rand";
}

template <typename T>
constexpr const char* random_engine_name() requires std::is_same_v<std::remove_cv_t<T>, std::minstd_rand0>
{
        return "std::minstd_rand0";
}

template <typename T>
constexpr const char* random_engine_name() requires std::is_same_v<std::remove_cv_t<T>, std::mt19937>
{
        return "std::mt19937";
}

template <typename T>
constexpr const char* random_engine_name() requires std::is_same_v<std::remove_cv_t<T>, std::mt19937_64>
{
        return "std::mt19937_64";
}

template <typename T>
constexpr const char* random_engine_name() requires std::is_same_v<std::remove_cv_t<T>, std::ranlux24>
{
        return "std::ranlux24";
}

template <typename T>
constexpr const char* random_engine_name() requires std::is_same_v<std::remove_cv_t<T>, std::ranlux24_base>
{
        return "std::ranlux24_base";
}

template <typename T>
constexpr const char* random_engine_name() requires std::is_same_v<std::remove_cv_t<T>, std::ranlux48>
{
        return "std::ranlux48";
}

template <typename T>
constexpr const char* random_engine_name() requires std::is_same_v<std::remove_cv_t<T>, std::ranlux48_base>
{
        return "std::ranlux48_base";
}
}
