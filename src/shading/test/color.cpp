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

#include <cmath>
#include <random>

namespace ns::shading::test
{
void check_color(const Color& color, const char* description)
{
        if (color.is_black())
        {
                error(std::string(description) + " is black");
        }

        const Vector<3, double> rgb = color.rgb<double>();

        for (int i = 0; i < 3; ++i)
        {
                if (std::isnan(rgb[i]))
                {
                        error(std::string(description) + " RGB is NaN " + to_string(rgb));
                }

                if (!std::isfinite(rgb[i]))
                {
                        error(std::string(description) + " RGB is not finite " + to_string(rgb));
                }

                if (!(rgb[i] >= 0))
                {
                        error(std::string(description) + " RGB is negative " + to_string(rgb));
                }
        }
}

void check_color_equal(const Color& directional_albedo, const Color& surface_color)
{
        check_color(directional_albedo, "Directional albedo");
        check_color(surface_color, "Surface color");

        const Vector<3, double> c1 = directional_albedo.rgb<double>();
        const Vector<3, double> c2 = surface_color.rgb<double>();

        for (int i = 0; i < 3; ++i)
        {
                if (c1[i] == c2[i])
                {
                        continue;
                }

                double relative_error = std::abs(c1[i] - c2[i]) / std::max(c1[i], c2[i]);
                if (!(relative_error < 0.01))
                {
                        error("BRDF error, directional albedo (RGB " + to_string(c1)
                              + ") is not equal to surface color (RGB " + to_string(c2) + ")");
                }
        }
}

void check_color_less(const Color& directional_albedo, const Color& surface_color)
{
        check_color(directional_albedo, "Directional albedo");
        check_color(surface_color, "Surface color");

        const Vector<3, double> c1 = directional_albedo.rgb<double>();
        const Vector<3, double> c2 = surface_color.rgb<double>();

        for (int i = 0; i < 3; ++i)
        {
                if (c1[i] <= c2[i])
                {
                        continue;
                }

                double relative_error = std::abs(c1[i] - c2[i]) / std::max(c1[i], c2[i]);
                if (!(relative_error < 0.01))
                {
                        error("BRDF error, directional albedo (RGB " + to_string(c1)
                              + ") is not less than surface color (RGB " + to_string(c2) + ")");
                }
        }
}

void check_color_range(const Color& directional_albedo)
{
        check_color(directional_albedo, "Directional albedo");

        const Vector<3, double> c = directional_albedo.rgb<double>();

        for (int i = 0; i < 3; ++i)
        {
                if (c[i] >= 0 && c[i] <= 1)
                {
                        continue;
                }

                error("BRDF error, directional albedo (RGB " + to_string(c) + ") is not in the range [0, 1]");
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
