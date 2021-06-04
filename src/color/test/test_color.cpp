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

#include "../color.h"

#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/test/test.h>

#include <cmath>
#include <random>

namespace ns::color
{
namespace
{
bool equal(const Vector<3, float>& rgb_1, const Vector<3, float>& rgb_2, float max_error, float sum_max_error)
{
        float abs_sum = 0;
        for (int i = 0; i < 3; ++i)
        {
                float abs = std::abs(rgb_1[i] - rgb_2[i]);
                if (!(abs <= max_error))
                {
                        return false;
                }
                abs_sum += abs;
        }
        return abs_sum <= sum_max_error;
}

template <typename ColorType>
void test_color_white_light(
        const std::string_view& test_name,
        std::mt19937_64& engine,
        float max_error,
        float sum_max_error)
{
        std::uniform_real_distribution<float> urd(0, 1);
        const Vector<3, float> rgb(urd(engine), urd(engine), urd(engine));

        const ColorType white_light = ColorType(1, 1, 1, Type::Illumination);
        const ColorType color(rgb[0], rgb[1], rgb[2]);

        const ColorType shaded = color * white_light;

        const Vector<3, float> shaded_rgb = shaded.rgb32();
        if (!equal(rgb, shaded_rgb, max_error, sum_max_error))
        {
                error(std::string(test_name) + ": white light values are not equal: RGB " + to_string(rgb)
                      + ", shaded RGB " + to_string(shaded_rgb));
        }
}

template <typename ColorType>
void test_color_white_color(
        const std::string_view& test_name,
        std::mt19937_64& engine,
        float max_error,
        float sum_max_error)
{
        std::uniform_real_distribution<float> urd(0, 1);
        const Vector<3, float> rgb(urd(engine), urd(engine), urd(engine));

        const ColorType white_color = ColorType(1, 1, 1);
        const ColorType light(rgb[0], rgb[1], rgb[2], Type::Illumination);

        const ColorType shaded = white_color * light;

        const Vector<3, float> shaded_rgb = shaded.rgb32();
        if (!equal(rgb, shaded_rgb, max_error, sum_max_error))
        {
                error(std::string(test_name) + ": white color values are not equal: RGB " + to_string(rgb)
                      + ", shaded RGB " + to_string(shaded_rgb));
        }
}

template <typename ColorType>
void test_color_constructors(const std::string_view& test_name, std::mt19937_64& engine, float max_error)
{
        std::uniform_real_distribution<float> urd(0, 1);
        const Vector<3, float> rgb(urd(engine), urd(engine), urd(engine));

        {
                const ColorType c1(rgb[0], rgb[1], rgb[2]);
                const ColorType c2(rgb[0], rgb[1], rgb[2], Type::Reflectance);
                if (!(c1 == c2))
                {
                        error(std::string(test_name)
                              + ": error default color type parameter, color with default parameter " + to_string(c1)
                              + ", color with reflectance parameter " + to_string(c2));
                }
        }

        const std::uint8_t r = linear_float_to_srgb_uint8(rgb[0]);
        const std::uint8_t g = linear_float_to_srgb_uint8(rgb[1]);
        const std::uint8_t b = linear_float_to_srgb_uint8(rgb[2]);
        {
                const ColorType c1(rgb[0], rgb[1], rgb[2]);
                const ColorType c2(RGB8(r, g, b));
                if (!(c1.equal_to_absolute(c2, max_error)))
                {
                        error(std::string(test_name) + ": error color constructors: RGB " + to_string(c1) + ", RGB8 "
                              + to_string(c2));
                }
        }
        {
                const ColorType c1(rgb[0], rgb[1], rgb[2], Type::Illumination);
                const ColorType c2(RGB8(r, g, b), Type::Illumination);
                if (!(c1.equal_to_absolute(c2, max_error)))
                {
                        error(std::string(test_name) + " error color constructors illumination: RGB " + to_string(c1)
                              + ", RGB8 " + to_string(c2));
                }
        }

        std::uniform_real_distribution<float> urd_n(-1, 1);
        {
                const ColorType color(urd_n(engine), urd_n(engine), urd_n(engine));
                if (!color.is_non_negative())
                {
                        error(std::string(test_name) + " color is not non-negative " + to_string(color));
                }
        }
        {
                const ColorType color(urd_n(engine));
                if (!color.is_non_negative())
                {
                        error(std::string(test_name) + " color is not non-negative " + to_string(color));
                }
        }
}

template <typename ColorType>
void test_color_luminance(const std::string_view& test_name, std::mt19937_64& engine)
{
        std::uniform_real_distribution<double> urd(0, 2);
        const double value = urd(engine);

        const ColorType color(value);

        const typename ColorType::DataType luminance = color.luminance();
        if (!(luminance >= 0))
        {
                error(std::string(test_name) + " luminance is not non-negative : luminance " + to_string(luminance)
                      + ", color " + to_string(color));
        }
        if (!(std::abs(value - luminance) <= 0.0002))
        {
                error(std::string(test_name) + " error color luminance: value " + to_string(value)
                      + ", color luminance " + to_string(luminance) + ", color " + to_string(color));
        }
}

template <typename ColorType>
void test_color(
        const std::string_view& test_name,
        float white_light_max_error,
        float white_light_sum_max_error,
        float white_color_max_error,
        float white_color_sum_max_error,
        float constructors_max_error)
{
        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        constexpr int COUNT = 1000;

        for (int i = 0; i < COUNT; ++i)
        {
                test_color_white_light<ColorType>(test_name, engine, white_light_max_error, white_light_sum_max_error);
        }

        for (int i = 0; i < COUNT; ++i)
        {
                test_color_white_color<ColorType>(test_name, engine, white_color_max_error, white_color_sum_max_error);
        }

        for (int i = 0; i < COUNT; ++i)
        {
                test_color_constructors<ColorType>(test_name, engine, constructors_max_error);
        }

        for (int i = 0; i < COUNT; ++i)
        {
                test_color_luminance<ColorType>(test_name, engine);
        }
}

template <typename T>
void test_color()
{
        test_color<ns::RGB>("Default RGB", 0, 0, 0, 0, 0.005);
        test_color<RGB<T>>("RGB", 0, 0, 0, 0, 0.005);

        test_color<ns::Spectrum>("Default Spectrum", 0.03, 0.05, 0.06, 0.07, 0.01);
        test_color<Spectrum<T, 50>>("Spectrum 50", 0.03, 0.05, 0.06, 0.07, 0.01);
        test_color<Spectrum<T, 64>>("Spectrum 64", 0.03, 0.05, 0.06, 0.07, 0.01);
        test_color<Spectrum<T, 100>>("Spectrum 100", 0.03, 0.05, 0.06, 0.07, 0.01);
        test_color<Spectrum<T, 128>>("Spectrum 128", 0.03, 0.05, 0.06, 0.07, 0.01);
}

void test()
{
        LOG("Test color");

        test_color<float>();
        test_color<double>();

        LOG("Test color passed");
}

TEST_SMALL("Color", test)
}
}
