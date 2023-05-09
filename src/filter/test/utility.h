/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <src/com/constant.h>
#include <src/com/file/path.h>
#include <src/numerical/vector.h>
#include <src/settings/directory.h>

#include <cctype>
#include <cmath>
#include <filesystem>
#include <optional>
#include <string>

namespace ns::filter::test
{
template <typename T, typename Angle>
Vector<2, T> rotate(const Vector<2, T>& v, const Angle angle)
{
        static_assert(std::is_floating_point_v<Angle>);

        return {std::cos(angle) * v[0] - std::sin(angle) * v[1], std::sin(angle) * v[0] + std::cos(angle) * v[1]};
}

inline std::string replace_space(const std::string_view s)
{
        std::string res;
        res.reserve(s.size());
        for (const char c : s)
        {
                res += !std::isspace(static_cast<unsigned char>(c)) ? c : '_';
        }
        return res;
}

inline std::filesystem::path test_file_path(const std::string_view name)
{
        return settings::test_directory() / path_from_utf8(name);
}

template <typename T>
T unbound_angle(const std::optional<T> previous, const T next)
{
        static constexpr T TWO_PI = 2 * PI<T>;
        if (previous)
        {
                return next - TWO_PI * std::round((next - *previous) / TWO_PI);
        }
        return next;
}

template <typename T>
T normalize_angle_difference(T difference)
{
        difference = std::fmod(difference, 2 * PI<T>);
        if (std::abs(difference) <= PI<T>)
        {
                return difference;
        }
        return (difference > 0) ? (difference - 2 * PI<T>) : (difference + 2 * PI<T>);
}
}
