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
#include "color.h"

#include "../ggx_diffuse.h"
#include "../lambertian.h"

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

template <std::size_t N, typename T>
class TestLambertian final : public TestBRDF<N, T, RandomEngine<T>>
{
        const Color m_color = random_non_black_color();

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
                return GGXDiffuseBRDF<N, T>::f(m_metalness, m_roughness, m_color, n, v, l);
        }

        Sample<N, T> sample_f(RandomEngine<T>& random_engine, const Vector<N, T>& n, const Vector<N, T>& v)
                const override
        {
                return GGXDiffuseBRDF<N, T>::sample_f(random_engine, m_metalness, m_roughness, m_color, n, v);
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
                const TestGGXDiffuse<N, T> brdf(random_non_black_color(), MIN_ROUGHNESS);

                Color result;

                LOG(to_string(N) + "D, " + type_name<T>() + ", GGX BRDF, f, random");
                result = test_brdf_f(brdf, SAMPLE_COUNT);
                check_color_range(result);

                LOG(to_string(N) + "D, " + type_name<T>() + ", GGX BRDF, sample f, random");
                result = test_brdf_sample_f(brdf, SAMPLE_COUNT);
                check_color_range(result);
        }
}

template <std::size_t N, typename T>
void test_brdf()
{
        test_lambertian<N, T>();
        test_ggx_diffuse<N, T>();
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
