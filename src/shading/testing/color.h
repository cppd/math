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

#include <src/com/error.h>

#include <string>

namespace ns::shading::testing
{
template <typename Color>
void check_color(const Color& color, const char* const description)
{
        if (color.is_black())
        {
                error(std::string(description) + " is black " + to_string(color));
        }

        if (color.has_nan())
        {
                error(std::string(description) + " has NaN " + to_string(color));
        }

        if (!color.is_finite())
        {
                error(std::string(description) + " is not finite " + to_string(color));
        }

        if (!color.is_non_negative())
        {
                error(std::string(description) + " is not non-negative " + to_string(color));
        }
}

template <typename Color>
void check_color_equal(const Color& directional_albedo, const Color& surface_color)
{
        check_color(directional_albedo, "Directional albedo");
        check_color(surface_color, "Surface color");

        constexpr double RELATIVE_ERROR = 0.01;

        if (!directional_albedo.equal_to_relative(surface_color, RELATIVE_ERROR))
        {
                error("BRDF error, directional albedo is not equal to surface color\n" + to_string(directional_albedo)
                      + "\n" + to_string(surface_color));
        }
}

template <typename Color>
void check_color_less(const Color& directional_albedo, const Color& surface_color)
{
        check_color(directional_albedo, "Directional albedo");
        check_color(surface_color, "Surface color");

        constexpr double RELATIVE_ERROR = 0.01;

        if (!directional_albedo.less_than(surface_color, RELATIVE_ERROR))
        {
                error("BRDF error, directional albedo is not less than surface color\n" + to_string(directional_albedo)
                      + "\n" + to_string(surface_color));
        }
}

template <typename Color>
void check_color_range(const Color& directional_albedo)
{
        check_color(directional_albedo, "Directional albedo");

        if (!directional_albedo.is_in_range(0, 1))
        {
                error("BRDF error, directional albedo is not in the range [0, 1] " + to_string(directional_albedo));
        }
}

template <typename Color, typename DescriptionFunction>
void check_uniform_importance_equal(
        const Color& uniform_sampling_albedo,
        const Color& importance_sampling_albedo,
        const double relative_error,
        const DescriptionFunction& description)
{
        check_color(uniform_sampling_albedo, "Uniform sampling directional albedo");
        check_color(importance_sampling_albedo, "Importance sampling directional albedo");

        if (!uniform_sampling_albedo.equal_to_relative(importance_sampling_albedo, relative_error))
        {
                std::string s;
                s += "BRDF error, uniform sampling directional albedo"
                     " is not equal to importance sampling directional albedo\n";
                s += to_string(uniform_sampling_albedo);
                s += "\n";
                s += to_string(importance_sampling_albedo);
                if (const std::string& d = description(); !d.empty())
                {
                        s += "\n";
                        s += d;
                }
                error(s);
        }
}
}
