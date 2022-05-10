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

#pragma once

#include <src/com/error.h>
#include <src/com/type/name.h>
#include <src/numerical/vector.h>

#include <charconv>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace ns::mesh::file
{
namespace data_read_implementation
{
template <typename T>
std::tuple<T, const char*> str_to_floating_point(const char* const str) requires(std::is_same_v<T, float>)
{
        char* end;
        const T v = std::strtof(str, &end);
        return {v, end};
}

template <typename T>
std::tuple<T, const char*> str_to_floating_point(const char* const str) requires(std::is_same_v<T, double>)
{
        char* end;
        const T v = std::strtod(str, &end);
        return {v, end};
}

template <typename T>
std::tuple<T, const char*> str_to_floating_point(const char* const str) requires(std::is_same_v<T, long double>)
{
        char* end;
        const T v = std::strtold(str, &end);
        return {v, end};
}

template <typename T>
std::tuple<std::optional<T>, const char*> read_float(const char* const str)
{
        const auto [value, ptr] = str_to_floating_point<T>(str);

        if (ptr > str)
        {
                if (std::isfinite(value))
                {
                        return {value, ptr};
                }
                error("Error reading " + std::string(type_name<T>()));
        }
        return {std::nullopt, str};
}
}

//

template <typename Iter, typename Op>
[[nodiscard]] Iter read(Iter first, const Iter last, const Op& op)
{
        while (first < last && op(*first))
        {
                ++first;
        }
        return first;
}

template <typename T>
[[nodiscard]] std::tuple<std::optional<T>, const char*> read_integer(const char* const first, const char* const last)
{
        static_assert(std::is_integral_v<T>);

        T v;
        const auto [ptr, ec] = std::from_chars(first, last, v);

        if (ec == std::errc{})
        {
                return {v, ptr};
        }

        if (ptr == first)
        {
                return {std::nullopt, first};
        }

        error("Error reading integral from " + std::string(first, last));
}

template <std::size_t N, typename T>
const char* read_float(const char* str, Vector<N, T>* const v)
{
        namespace impl = data_read_implementation;

        for (std::size_t i = 0; i < N; ++i)
        {
                const auto [value, ptr] = impl::read_float<T>(str);
                if (value)
                {
                        (*v)[i] = *value;
                        str = ptr;
                        continue;
                }
                error("Error reading " + std::string(type_name<T>()));
        }

        return str;
}

template <std::size_t N, typename T>
const char* read_float(const char* str, Vector<N, T>* const v, std::optional<T>* const n)
{
        namespace impl = data_read_implementation;

        str = read_float(str, v);

        std::tie(*n, str) = impl::read_float<T>(str);

        return str;
}
}
