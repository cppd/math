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

#include "../ggx_diffuse.h"
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

namespace ns::shading::test
{
namespace
{
template <typename T>
using RandomEngine = std::conditional_t<sizeof(T) <= 4, std::mt19937, std::mt19937_64>;

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

Color random_color()
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

template <std::size_t N, typename T>
class TestLambertian final : public TestBRDF<N, T, RandomEngine<T>>
{
        const Color m_color = random_color();

        Color f(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
        {
                if (dot(n, v) <= 0)
                {
                        return Color(0);
                }
                return LambertianBRDF<N, T>::f(m_color, n, l);
        }

        Sample<N, T> sample_f(RandomEngine<T>& random_engine, const Vector<N, T>& n, const Vector<N, T>& v)
                const override
        {
                if (dot(n, v) <= 0)
                {
                        return {Vector<N, T>(0), 0, Color(0)};
                }
                return LambertianBRDF<N, T>::sample_f(random_engine, m_color, n);
        }

public:
        const Color& color() const
        {
                return m_color;
        }
};

template <std::size_t N, typename T>
class TestGGXDiffuse final : public TestBRDF<N, T, RandomEngine<T>>
{
        Color m_color;
        T m_metalness;
        T m_roughness;

        Color f(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
        {
                return GGXDiffuseBRDF<T>::f(m_metalness, m_roughness, m_color, n, v, l);
        }

        Sample<N, T> sample_f(RandomEngine<T>& random_engine, const Vector<N, T>& n, const Vector<N, T>& v)
                const override
        {
                return GGXDiffuseBRDF<T>::sample_f(random_engine, m_metalness, m_roughness, m_color, n, v);
        }

public:
        TestGGXDiffuse(const Color& color, std::type_identity_t<T> min_roughness)
        {
                m_color = color;
                RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();
                m_roughness = std::uniform_real_distribution<T>(min_roughness, 1)(random_engine);
                m_metalness = std::uniform_real_distribution<T>(0, 1)(random_engine);
        }

        const Color& color() const
        {
                return m_color;
        }
};

template <std::size_t N, typename T>
void test_lambertian()
{
        constexpr unsigned SAMPLE_COUNT = 100'000;

        const TestLambertian<N, T> brdf;

        Color result;

        LOG(to_string(N) + "D, " + type_name<T>() + ", Lambertian BRDF, f");
        result = test_brdf_f(brdf, SAMPLE_COUNT);
        check_color_equal(result, brdf.color());

        LOG(to_string(N) + "D, " + type_name<T>() + ", Lambertian BRDF, sample f");
        result = test_brdf_sample_f(brdf, SAMPLE_COUNT);
        check_color_equal(result, brdf.color());
}

template <std::size_t N, typename T>
void test_ggx_diffuse()
{
        constexpr unsigned SAMPLE_COUNT = 1'000'000;
        constexpr T MIN_ROUGHNESS = 0.2;

        {
                const TestGGXDiffuse<N, T> brdf(Color(1), MIN_ROUGHNESS);

                Color result;

                LOG(to_string(N) + "D, " + type_name<T>() + ", GGX BRDF, f, white");
                result = test_brdf_f(brdf, SAMPLE_COUNT);
                check_color_less(result, brdf.color());

                LOG(to_string(N) + "D, " + type_name<T>() + ", GGX BRDF, sample f, white");
                result = test_brdf_sample_f(brdf, SAMPLE_COUNT);
                check_color_less(result, brdf.color());
        }
        {
                const TestGGXDiffuse<N, T> brdf(random_color(), MIN_ROUGHNESS);

                Color result;

                LOG(to_string(N) + "D, " + type_name<T>() + ", GGX BRDF, f, random");
                result = test_brdf_f(brdf, SAMPLE_COUNT);
                check_color_range(result);

                LOG(to_string(N) + "D, " + type_name<T>() + ", GGX BRDF, sample f, random");
                result = test_brdf_sample_f(brdf, SAMPLE_COUNT);
                check_color_range(result);
        }
}

template <typename T>
void test_brdf()
{
        test_lambertian<3, T>();
        test_ggx_diffuse<3, T>();

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
