/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "error.h"

#include "type/name.h"

#include <charconv>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <string>
#include <system_error>
#include <tuple>

namespace ns
{
namespace read_implementation
{
template <typename T>
        requires (std::is_same_v<T, float>)
std::tuple<T, const char*> read(const char* const str)
{
        char* end;
        const T v = std::strtof(str, &end);
        return {v, end};
}

template <typename T>
        requires (std::is_same_v<T, double>)
std::tuple<T, const char*> read(const char* const str)
{
        char* end;
        const T v = std::strtod(str, &end);
        return {v, end};
}

template <typename T>
        requires (std::is_same_v<T, long double>)
std::tuple<T, const char*> read(const char* const str)
{
        char* end;
        const T v = std::strtold(str, &end);
        return {v, end};
}
}

template <typename T>
        requires (std::is_floating_point_v<T>)
[[nodiscard]] std::tuple<std::optional<T>, const char*> read_from_chars(const char* const str)
{
        namespace impl = read_implementation;

        const auto [value, ptr] = impl::read<T>(str);

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

template <typename T>
        requires (std::is_integral_v<T>)
[[nodiscard]] std::tuple<std::optional<T>, const char*> read_from_chars(const char* const first, const char* const last)
{
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

        error("Error reading integral");
}
}
