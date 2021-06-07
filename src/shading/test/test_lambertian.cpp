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

#include "../lambertian.h"

#include <src/color/color.h>
#include <src/com/log.h>
#include <src/com/print.h>
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
        const Color m_color = random_non_black_color<Color>();

        Color f(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
        {
                if (dot(n, v) <= 0)
                {
                        return Color(0);
                }
                return LambertianBRDF<N, T>::f(m_color, n, l);
        }

        Sample<N, T, Color> sample_f(RandomEngine<T>& random_engine, const Vector<N, T>& n, const Vector<N, T>& v)
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

template <std::size_t N, typename T, typename Color>
void test_brdf()
{
        constexpr unsigned SAMPLE_COUNT = 100'000;

        const BRDF<N, T, Color> brdf;

        Color result;

        LOG(Color::name() + ", " + to_string(N) + "D, " + type_name<T>() + ", f");
        result = test_brdf_f(brdf, SAMPLE_COUNT);
        check_color_equal(result, brdf.color());

        LOG(Color::name() + ", " + to_string(N) + "D, " + type_name<T>() + ", sample f");
        result = test_brdf_sample_f(brdf, SAMPLE_COUNT);
        check_color_equal(result, brdf.color());
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
        LOG("Test Lambertian BRDF");

        test_brdf<Color>();
        test_brdf<Spectrum>();

        LOG("Test Lambertian BRDF passed");
}
}

TEST_SMALL("BRDF, Lambertian", test)
}
