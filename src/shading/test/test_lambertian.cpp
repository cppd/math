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
#include "compute.h"

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
class TestBRDF final : public BRDF<N, T, Color, RandomEngine<T>>
{
        const Color color_ = random_non_black_color<Color>();

        Color f(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
        {
                if (dot(n, v) <= 0)
                {
                        return Color(0);
                }
                return lambertian::f(color_, n, l);
        }

        T pdf(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
        {
                if (dot(n, v) <= 0)
                {
                        return 0;
                }
                return lambertian::pdf(n, l);
        }

        Sample<N, T, Color> sample_f(RandomEngine<T>& random_engine, const Vector<N, T>& n, const Vector<N, T>& v)
                const override
        {
                if (dot(n, v) <= 0)
                {
                        return {Vector<N, T>(0), 0, Color(0)};
                }
                return lambertian::sample_f(random_engine, color_, n);
        }

public:
        const Color& color() const
        {
                return color_;
        }
};

template <std::size_t N, typename T, typename Color>
void test_brdf()
{
        constexpr unsigned SAMPLE_COUNT = 100'000;

        const TestBRDF<N, T, Color> brdf;

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", uniform");
        {
                const Color color = directional_albedo_uniform_sampling(brdf, SAMPLE_COUNT);
                check_color_equal(color, brdf.color());
        }

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", PDF integral");
        {
                const T integral = directional_pdf_integral(brdf, SAMPLE_COUNT);
                if (!(std::abs(integral - 1) <= T(0.01)))
                {
                        error("BRDF error, PDF integral is not equal to 1\n" + to_string(integral));
                }
        }

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", importance");
        {
                const Color color = directional_albedo_importance_sampling(brdf, SAMPLE_COUNT);
                check_color_equal(color, brdf.color());
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
        LOG("Test Lambertian BRDF");

        test_brdf<color::Color>();
        test_brdf<color::Spectrum>();

        LOG("Test Lambertian BRDF passed");
}
}

TEST_SMALL("BRDF, Lambertian", test)
}
