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

#include "../lambertian.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/name.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>
#include <random>

namespace ns::painter
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

template <template <std::size_t N, typename> typename BRDF, std::size_t N, typename T>
void test_brdf_f(const BRDF<N, T>& brdf, const unsigned sample_count)
{
        const T UNIFORM_ON_HEMISPHERE_PDF = 2 * sampling::uniform_on_sphere_pdf<N, T>();

        RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();

        const Color TEST_COLOR = [&]
        {
                std::uniform_real_distribution<Color::DataType> urd(0, 1);
                Color::DataType red = urd(random_engine);
                Color::DataType green = urd(random_engine);
                Color::DataType blue = urd(random_engine);
                return Color(red, green, blue);
        }();

        const Vector<N, T> n = sampling::uniform_on_sphere<N, T>(random_engine);

        Color sum(0);
        unsigned sample = 0;

        while (sample < sample_count)
        {
                const Vector<N, T> l = sampling::uniform_on_sphere<N, T>(random_engine);
                const T n_l = dot(n, l);

                if (n_l <= 0)
                {
                        Color c = brdf.f(TEST_COLOR, n, l);
                        if (!c.is_black())
                        {
                                error("BRDF color " + to_string(c.rgb<float>()) + " is not black when dot(n,l) <= 0");
                        }
                        continue;
                }

                ++sample;
                Color c = brdf.f(TEST_COLOR, n, l);
                if (c.is_black())
                {
                        continue;
                }
                sum += c * (n_l / UNIFORM_ON_HEMISPHERE_PDF);
        }

        check_color_equal(sum / sample_count, TEST_COLOR);
}

template <template <std::size_t N, typename> typename BRDF, std::size_t N, typename T>
void test_brdf_sample_f(const BRDF<N, T>& brdf, const unsigned sample_count)
{
        RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();

        const Color TEST_COLOR = [&]
        {
                std::uniform_real_distribution<Color::DataType> urd(0, 1);
                Color::DataType red = urd(random_engine);
                Color::DataType green = urd(random_engine);
                Color::DataType blue = urd(random_engine);
                return Color(red, green, blue);
        }();

        const Vector<N, T> n = sampling::uniform_on_sphere<N, T>(random_engine);

        Color sum(0);

        for (unsigned i = 0; i < sample_count; ++i)
        {
                BrdfSample<N, T> sample = brdf.sample_f(random_engine, TEST_COLOR, n);
                if (sample.brdf.is_black() || sample.pdf <= 0)
                {
                        continue;
                }
                const T n_l = dot(n, sample.l);
                if (n_l <= 0)
                {
                        continue;
                }
                sum += sample.brdf * (n_l / sample.pdf);
        }

        check_color_equal(sum / sample_count, TEST_COLOR);
}

template <std::size_t N, typename T>
void test_brdf()
{
        const LambertianBRDF<N, T> brdf;
        constexpr unsigned SAMPLE_COUNT = 100'000;

        LOG(to_string(N) + "D, " + type_name<T>() + ", Lambertian BRDF, f");
        test_brdf_f(brdf, SAMPLE_COUNT);

        LOG(to_string(N) + "D, " + type_name<T>() + ", Lambertian BRDF, sample f");
        test_brdf_sample_f(brdf, SAMPLE_COUNT);
}

template <typename T>
void test_brdf()
{
        test_brdf<3, T>();
        test_brdf<4, T>();
        test_brdf<5, T>();
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
