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

#include "brdf.h"

#include "../lambertian.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/name.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>
#include <random>

namespace ns::painter::brdf::test
{
namespace
{
void check_color_equal(const Color& directional_albedo, const Color& test)
{
        if (directional_albedo.is_black() && test.is_black())
        {
                return;
        }

        const Vector<3, double> c1 = directional_albedo.rgb<double>();
        const Vector<3, double> c2 = test.rgb<double>();

        for (int i = 0; i < 3; ++i)
        {
                if (!(c1[i] >= 0))
                {
                        error("RGB is negative " + to_string(c1));
                }

                if (!(c2[i] >= 0))
                {
                        error("RGB is negative " + to_string(c2));
                }

                if (c1[i] == c2[i])
                {
                        continue;
                }

                double relative_error = std::abs(c1[i] - c2[i]) / std::max(c1[i], c2[i]);
                if (!(relative_error < 0.01))
                {
                        error("BRDF error, directional albedo (RGB " + to_string(c1)
                              + ") is not equal to test color (RGB " + to_string(c2) + ")");
                }
        }
}

Color random_color()
{
        std::mt19937 random_engine = create_engine<std::mt19937>();
        std::uniform_real_distribution<Color::DataType> urd(0, 1);

        Color::DataType red = urd(random_engine);
        Color::DataType green = urd(random_engine);
        Color::DataType blue = urd(random_engine);

        return Color(red, green, blue);
}

template <std::size_t N, typename T>
class TestLambertian final : public TestBRDF<N, T>
{
        Color m_color;

        Color f(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
        {
                if (dot(n, v) <= 0)
                {
                        return Color(0);
                }
                return LambertianBRDF<N, T>::f(m_color, n, l);
        }

        BrdfSample<N, T> sample_f(RandomEngine<T>& random_engine, const Vector<N, T>& n, const Vector<N, T>& v)
                const override
        {
                if (dot(n, v) <= 0)
                {
                        return {Vector<N, T>(0), 0, Color(0)};
                }
                return LambertianBRDF<N, T>::sample_f(random_engine, m_color, n);
        }

public:
        explicit TestLambertian(const Color& color) : m_color(color)
        {
        }
};

template <std::size_t N, typename T>
void test_lambertian()
{
        constexpr unsigned SAMPLE_COUNT = 100'000;

        const Color color = random_color();

        TestLambertian<N, T> brdf(color);

        Color result;

        LOG(to_string(N) + "D, " + type_name<T>() + ", Lambertian BRDF, f");
        result = test_brdf_f(brdf, SAMPLE_COUNT);
        check_color_equal(result, color);

        LOG(to_string(N) + "D, " + type_name<T>() + ", Lambertian BRDF, sample f");
        result = test_brdf_sample_f(brdf, SAMPLE_COUNT);
        check_color_equal(result, color);
}

template <typename T>
void test_brdf()
{
        test_lambertian<3, T>();
        test_lambertian<4, T>();
        test_lambertian<5, T>();
}

void test()
{
        LOG("Test BRDF");

        test_brdf<float>();
        test_brdf<double>();

        LOG("Test BRDF passed");
}
}

TEST_SMALL("BRDF", test)
}
