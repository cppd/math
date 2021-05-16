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

#include "color.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>

#include <random>

namespace ns::shading::test
{
namespace
{
void check_color(const Color& color, const char* description)
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
}

void check_color_equal(const Color& directional_albedo, const Color& surface_color)
{
        check_color(directional_albedo, "Directional albedo");
        check_color(surface_color, "Surface color");

        constexpr Color::DataType RELATIVE_ERROR = 0.01;

        if (!directional_albedo.equal_to(surface_color, RELATIVE_ERROR))
        {
                error("BRDF error, directional albedo is not equal to surface color\n" + to_string(directional_albedo)
                      + "\n" + to_string(surface_color));
        }
}

void check_color_less(const Color& directional_albedo, const Color& surface_color)
{
        check_color(directional_albedo, "Directional albedo");
        check_color(surface_color, "Surface color");

        constexpr Color::DataType RELATIVE_ERROR = 0.01;

        if (!directional_albedo.less_than(surface_color, RELATIVE_ERROR))
        {
                error("BRDF error, directional albedo is not less than surface color\n" + to_string(directional_albedo)
                      + "\n" + to_string(surface_color));
        }
}

void check_color_range(const Color& directional_albedo)
{
        check_color(directional_albedo, "Directional albedo");

        if (!directional_albedo.is_in_range(0, 1))
        {
                error("BRDF error, directional albedo is not in the range [0, 1] " + to_string(directional_albedo));
        }
}

Color random_non_black_color()
{
        std::mt19937 random_engine = create_engine<std::mt19937>();
        std::uniform_real_distribution<Color::DataType> urd(0, 1);

        Color color;
        do
        {
                Color::DataType red = urd(random_engine);
                Color::DataType green = urd(random_engine);
                Color::DataType blue = urd(random_engine);
                color = Color(red, green, blue);
        } while (color.is_black());

        return color;
}
}
