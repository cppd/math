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

namespace ns::filter::test::utility
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

template <std::size_t N, typename T>
[[nodiscard]] T is_positive(const Vector<N, T>& v)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!(v[i] > 0))
                {
                        return false;
                }
        }
        return true;
}

template <typename T>
[[nodiscard]] T compute_angle(const Vector<2, T>& velocity)
{
        return std::atan2(velocity[1], velocity[0]);
}

template <typename T>
[[nodiscard]] T compute_angle_p(const Vector<2, T>& velocity, const Matrix<2, 2, T>& velocity_p)
{
        // angle = atan(y/x)
        // Jacobian
        //  -y/(x*x+y*y) x/(x*x+y*y)
        const T ns = velocity.norm_squared();
        const T x = velocity[0];
        const T y = velocity[1];
        const Matrix<1, 2, T> error_propagation{
                {-y / ns, x / ns}
        };
        const Matrix<1, 1, T> p = error_propagation * velocity_p * error_propagation.transposed();
        return p(0, 0);
}

template <std::size_t N, typename T>
[[nodiscard]] T compute_speed_p(const Vector<N, T>& velocity, const Matrix<N, N, T>& velocity_p)
{
        // speed = sqrt(vx*vx + vy*vy)
        // Jacobian
        //  x/sqrt(x*x+y*y) y/sqrt(x*x+y*y)
        const Matrix<1, N, T> error_propagation(velocity.normalized());
        const Matrix<1, 1, T> p = error_propagation * velocity_p * error_propagation.transposed();
        return p(0, 0);
}
}
