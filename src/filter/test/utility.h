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
#include <src/numerical/matrix.h>
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
[[nodiscard]] Vector<2, T> rotate(const Vector<2, T>& v, const Angle angle)
{
        static_assert(std::is_floating_point_v<Angle>);

        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        const Matrix<2, 2, T> m{
                {cos, -sin},
                {sin,  cos}
        };
        return m * v;
}

[[nodiscard]] inline std::string replace_space(const std::string_view s)
{
        std::string res;
        res.reserve(s.size());
        for (const char c : s)
        {
                res += !std::isspace(static_cast<unsigned char>(c)) ? c : '_';
        }
        return res;
}

[[nodiscard]] inline std::filesystem::path test_file_path(const std::string_view name)
{
        return settings::test_directory() / path_from_utf8(name);
}

template <typename T>
[[nodiscard]] T normalize_angle(const T difference)
{
        return std::remainder(difference, 2 * PI<T>);
}

template <typename T>
[[nodiscard]] T unbound_angle(const std::optional<T> previous, const T next)
{
        if (previous)
        {
                return *previous + normalize_angle(next - *previous);
        }
        return next;
}
}
