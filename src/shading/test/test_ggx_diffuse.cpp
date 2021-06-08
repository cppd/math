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

#include <src/color/color.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/name.h>
#include <src/test/test.h>

#include <random>

namespace ns::shading::test
{
namespace
{
template <typename T>
using RandomEngine = std::conditional_t<sizeof(T) <= 4, std::mt19937, std::mt19937_64>;

template <std::size_t N, typename T, typename Color>
class BRDF final : public TestBRDF<N, T, Color, RandomEngine<T>>
{
        Color m_color;
        T m_metalness;
        T m_roughness;

        Color f(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
        {
                return ggx_diffuse::f(m_metalness, m_roughness, m_color, n, v, l);
        }

        Sample<N, T, Color> sample_f(RandomEngine<T>& random_engine, const Vector<N, T>& n, const Vector<N, T>& v)
                const override
        {
                return ggx_diffuse::sample_f(random_engine, m_metalness, m_roughness, m_color, n, v);
        }

public:
        BRDF(const Color& color, std::type_identity_t<T> min_roughness)
        {
                m_color = color;
                RandomEngine<std::mt19937_64> random_engine = create_engine<std::mt19937_64>();
                m_roughness = std::uniform_real_distribution<T>(min_roughness, 1)(random_engine);
                m_metalness = std::uniform_real_distribution<T>(0, 1)(random_engine);
        }

        const Color& color() const
        {
                return m_color;
        }
};

template <std::size_t N, typename T, typename Color>
void test_brdf()
{
        constexpr unsigned SAMPLE_COUNT = 100'000;
        constexpr T MIN_ROUGHNESS = 0.2;

        {
                const BRDF<N, T, Color> brdf(Color(1), MIN_ROUGHNESS);

                Color result;

                LOG(Color::name() + ", " + to_string(N) + "D, " + type_name<T>() + ", f, white");
                result = test_brdf_f(brdf, SAMPLE_COUNT);
                check_color_less(result, brdf.color());

                LOG(Color::name() + ", " + to_string(N) + "D, " + type_name<T>() + ", sample f, white");
                result = test_brdf_sample_f(brdf, SAMPLE_COUNT);
                check_color_less(result, brdf.color());
        }
        {
                const BRDF<N, T, Color> brdf(random_non_black_color<Color>(), MIN_ROUGHNESS);

                Color result;

                LOG(Color::name() + ", " + to_string(N) + "D, " + type_name<T>() + ", f, random");
                result = test_brdf_f(brdf, SAMPLE_COUNT);
                check_color_range(result);

                LOG(Color::name() + ", " + to_string(N) + "D, " + type_name<T>() + ", sample f, random");
                result = test_brdf_sample_f(brdf, SAMPLE_COUNT);
                check_color_range(result);
        }
}

template <typename T, typename Color>
void test_brdf()
{
        test_brdf<3, T, Color>();
        test_brdf<4, T, Color>();
        test_brdf<5, T, Color>();
}

template <typename Color>
void test_brdf()
{
        test_brdf<float, Color>();
        test_brdf<double, Color>();
}

void test()
{
        LOG("Test GGX Diffuse BRDF");

        test_brdf<color::Color>();
        test_brdf<color::Spectrum>();

        LOG("Test GGX Diffuse BRDF passed");
}
}

TEST_SMALL("BRDF, GGX Diffuse", test)
}
