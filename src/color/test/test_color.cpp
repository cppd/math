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

#include "../color.h"
#include "../conversion.h"
#include "../rgb8.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/math.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

#include <cmath>
#include <cstdint>
#include <random>
#include <string>
#include <string_view>

namespace ns::color
{
namespace
{
template <typename T>
struct Check final
{
        static_assert(RGB<T>(1).rgb32() == Vector<3, float>(1, 1, 1));
        static_assert(RGB<T>(0).rgb32() == Vector<3, float>(0, 0, 0));
        static_assert(RGB<T>(0.1).rgb32() == Vector<3, float>(0.1));
        static_assert(RGB<T>(1, 1, 1).rgb32() == Vector<3, float>(1));
        static_assert(RGB<T>(0, 0, 0).rgb32() == Vector<3, float>(0));
        static_assert(RGB<T>(0.1, 0.2, 0.3).rgb32() == Vector<3, float>(0.1, 0.2, 0.3));
        static_assert(RGB<T>(RGB8(255, 255, 255)).rgb32() == Vector<3, float>(1, 1, 1));
        static_assert(RGB<T>(RGB8(0, 0, 0)).rgb32() == Vector<3, float>(0, 0, 0));
        static_assert(RGB<T>(RGB8(100, 150, 50)).rgb32() == RGB8(100, 150, 50).linear_rgb());
        static_assert(RGB<T>(RGB8(250, 10, 100)).rgb32() == RGB8(250, 10, 100).linear_rgb());
        static_assert(absolute(RGB<T>(RGB8(255, 255, 255)).luminance() - 1) < 1000 * Limits<T>::epsilon());
        static_assert(RGB<T>(0.1, 0.9, 0.2).luminance() == linear_float_to_linear_luminance<T>(0.1, 0.9, 0.2));
        static_assert(RGB<T>(0.9, 0.3, 0.8).luminance() == linear_float_to_linear_luminance<T>(0.9, 0.3, 0.8));

        static_assert(RGB<T>(0.1, 0.2, 0.3) == RGB<T>(0.1, 0.2, 0.3));
        static_assert(RGB<T>(0.1, 0.2, 0.3) != RGB<T>(0.4, 0.5, 0.6));

        static_assert(
                RGB<T>(0.1, 0.2, 0.3) + RGB<T>(0.4, 0.5, 0.6)
                == RGB<T>(T{0.1} + T{0.4}, T{0.2} + T{0.5}, T{0.3} + T{0.6}));
        static_assert(
                []
                {
                        RGB<T> res(0.1, 0.2, 0.3);
                        res += RGB<T>(0.4, 0.5, 0.6);
                        return res;
                }()
                == RGB<T>(T{0.1} + T{0.4}, T{0.2} + T{0.5}, T{0.3} + T{0.6}));

        static_assert(
                RGB<T>(0.4, 0.5, 0.6) - RGB<T>(0.1, 0.2, 0.3)
                == RGB<T>(T{0.4} - T{0.1}, T{0.5} - T{0.2}, T{0.6} - T{0.3}));
        static_assert(
                []
                {
                        RGB<T> res(0.4, 0.5, 0.6);
                        res -= RGB<T>(0.1, 0.2, 0.3);
                        return res;
                }()
                == RGB<T>(T{0.4} - T{0.1}, T{0.5} - T{0.2}, T{0.6} - T{0.3}));

        static_assert(
                RGB<T>(0.1, 0.2, 0.3) * RGB<T>(0.4, 0.5, 0.6)
                == RGB<T>(T{0.1} * T{0.4}, T{0.2} * T{0.5}, T{0.3} * T{0.6}));
        static_assert(
                []
                {
                        RGB<T> res(0.1, 0.2, 0.3);
                        res *= RGB<T>(0.4, 0.5, 0.6);
                        return res;
                }()
                == RGB<T>(T{0.1} * T{0.4}, T{0.2} * T{0.5}, T{0.3} * T{0.6}));

        static_assert(RGB<T>(0.1, 0.2, 0.3) * 4.1 == RGB<T>(T{0.1} * T{4.1}, T{0.2} * T{4.1}, T{0.3} * T{4.1}));
        static_assert(
                []
                {
                        RGB<T> res(0.1, 0.2, 0.3);
                        res *= 4.1;
                        return res;
                }()
                == RGB<T>(T{0.1} * T{4.1}, T{0.2} * T{4.1}, T{0.3} * T{4.1}));

        static_assert(RGB<T>(0.1, 0.2, 0.3) / 4.1 == RGB<T>(T{0.1} / T{4.1}, T{0.2} / T{4.1}, T{0.3} / T{4.1}));
        static_assert(
                []
                {
                        RGB<T> res(0.1, 0.2, 0.3);
                        res /= 4.1;
                        return res;
                }()
                == RGB<T>(T{0.1} / T{4.1}, T{0.2} / T{4.1}, T{0.3} / T{4.1}));

        static_assert(RGB<T>(0.1, 0.2, 0.3) * 4.1 == 4.1 * RGB<T>(0.1, 0.2, 0.3));
};
template struct Check<float>;
template struct Check<double>;
template struct Check<long double>;

bool equal(
        const Vector<3, float>& rgb_1,
        const Vector<3, float>& rgb_2,
        const float max_error,
        const float sum_max_error)
{
        float abs_sum = 0;
        for (int i = 0; i < 3; ++i)
        {
                const float abs = std::abs(rgb_1[i] - rgb_2[i]);
                if (!(abs <= max_error))
                {
                        return false;
                }
                abs_sum += abs;
        }
        return abs_sum <= sum_max_error;
}

template <typename RandomEngine>
Vector<3, float> random_rgb(RandomEngine& engine)
{
        std::uniform_real_distribution<float> urd(0, 1);
        return {urd(engine), urd(engine), urd(engine)};
}

template <typename ColorType, typename RandomEngine>
void test_color_white_light(
        RandomEngine& engine,
        const std::string_view test_name,
        const float max_error,
        const float sum_max_error)
{
        const Vector<3, float> rgb = random_rgb(engine);

        const ColorType white_light = ColorType::illuminant(1, 1, 1);
        const ColorType color(rgb[0], rgb[1], rgb[2]);

        const ColorType shaded = color * white_light;

        const Vector<3, float> shaded_rgb = shaded.rgb32();
        if (!equal(rgb, shaded_rgb, max_error, sum_max_error))
        {
                error(std::string(test_name) + ": white light values are not equal: RGB " + to_string(rgb)
                      + ", shaded RGB " + to_string(shaded_rgb));
        }
}

template <typename ColorType, typename RandomEngine>
void test_color_white_color(
        RandomEngine& engine,
        const std::string_view test_name,
        const float max_error,
        const float sum_max_error)
{
        const Vector<3, float> rgb = random_rgb(engine);

        const ColorType white_color(1, 1, 1);
        const ColorType light = ColorType::illuminant(rgb[0], rgb[1], rgb[2]);

        const ColorType shaded = white_color * light;

        const Vector<3, float> shaded_rgb = shaded.rgb32();
        if (!equal(rgb, shaded_rgb, max_error, sum_max_error))
        {
                error(std::string(test_name) + ": white color values are not equal: RGB " + to_string(rgb)
                      + ", shaded RGB " + to_string(shaded_rgb));
        }
}

template <typename ColorType, typename RandomEngine>
void test_color_constructors(RandomEngine& engine, const std::string_view test_name, const float max_error)
{
        const Vector<3, float> rgb = random_rgb(engine);

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
                const ColorType c1 = ColorType::illuminant(rgb[0], rgb[1], rgb[2]);
                const ColorType c2 = ColorType::illuminant(RGB8(r, g, b));
                if (!(c1.equal_to_absolute(c2, max_error)))
                {
                        error(std::string(test_name) + " error color constructors illumination: RGB " + to_string(c1)
                              + ", RGB8 " + to_string(c2));
                }
        }
}

template <typename ColorType, typename FloatType, typename RandomEngine>
void test_color_conversions(RandomEngine& engine, const std::string_view test_name)
{
        const Vector<3, float> rgb = random_rgb(engine);

        {
                const ColorType c1(rgb[0], rgb[1], rgb[2]);
                const auto c2 = to_color<ColorType>(RGB<FloatType>(rgb[0], rgb[1], rgb[2]));
                if (!(c1 == c2))
                {
                        error(std::string(test_name) + ": error to_color: RGB " + to_string(c1) + ", RGB "
                              + type_name<FloatType>() + " " + to_string(c2));
                }
        }
        {
                const ColorType c1 = ColorType::illuminant(rgb[0], rgb[1], rgb[2]);
                const auto c2 = to_illuminant<ColorType>(RGB<FloatType>(rgb[0], rgb[1], rgb[2]));
                if (!(c1 == c2))
                {
                        error(std::string(test_name) + ": error to_color illumination: RGB " + to_string(c1) + ", RGB "
                              + type_name<FloatType>() + " " + to_string(c2));
                }
        }
}

template <typename ColorType, typename RandomEngine>
void test_color_non_negative(RandomEngine& engine, const std::string_view test_name)
{
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

template <typename ColorType, typename RandomEngine>
void test_color_luminance(RandomEngine& engine, const std::string_view test_name)
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

template <typename ColorType, typename RandomEngine>
void test_color(
        RandomEngine& engine,
        const std::string_view test_name,
        const float white_light_max_error,
        const float white_light_sum_max_error,
        const float white_color_max_error,
        const float white_color_sum_max_error,
        const float constructors_max_error)
{
        test_color_white_light<ColorType>(engine, test_name, white_light_max_error, white_light_sum_max_error);
        test_color_white_color<ColorType>(engine, test_name, white_color_max_error, white_color_sum_max_error);
        test_color_constructors<ColorType>(engine, test_name, constructors_max_error);
        test_color_conversions<ColorType, float>(engine, test_name);
        test_color_conversions<ColorType, double>(engine, test_name);
        test_color_non_negative<ColorType>(engine, test_name);
        test_color_luminance<ColorType>(engine, test_name);
}

template <typename T, typename RandomEngine>
void test_color(RandomEngine& engine)
{
        test_color<RGB<T>>(engine, "RGB", 0, 0, 0, 0, 0.005);

        test_color<Color>(engine, "Default Color", 0.03, 0.05, 0.06, 0.07, 0.01);
        test_color<Spectrum>(engine, "Default Spectrum", 0.03, 0.05, 0.06, 0.07, 0.01);
        test_color<SpectrumSamples<T, 50>>(engine, "Spectrum 50", 0.03, 0.05, 0.06, 0.07, 0.01);
        test_color<SpectrumSamples<T, 64>>(engine, "Spectrum 64", 0.03, 0.05, 0.06, 0.07, 0.01);
        test_color<SpectrumSamples<T, 100>>(engine, "Spectrum 100", 0.03, 0.05, 0.06, 0.07, 0.01);
        test_color<SpectrumSamples<T, 128>>(engine, "Spectrum 128", 0.03, 0.05, 0.06, 0.07, 0.01);
}

void test()
{
        LOG("Test color");

        PCG engine;
        for (int i = 0; i < 1000; ++i)
        {
                test_color<float>(engine);
                test_color<double>(engine);
        }

        LOG("Test color passed");
}

TEST_SMALL("Color", test)
}
}
