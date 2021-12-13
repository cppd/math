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

template <typename T>
constexpr T MIN_ROUGHNESS = 0.3;

template <std::size_t N, typename T, typename Color>
class TestBRDF final : public BRDF<N, T, Color, RandomEngine<T>>
{
        Color color_;
        T metalness_;
        T roughness_;

        Color f(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
        {
                return ggx_diffuse::f(metalness_, roughness_, color_, n, v, l);
        }

        T pdf(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
        {
                return ggx_diffuse::pdf(roughness_, n, v, l);
        }

        Sample<N, T, Color> sample_f(RandomEngine<T>& random_engine, const Vector<N, T>& n, const Vector<N, T>& v)
                const override
        {
                return ggx_diffuse::sample_f(random_engine, metalness_, roughness_, color_, n, v);
        }

        TestBRDF(
                const Color& color,
                const std::type_identity_t<T>& min_roughness,
                RandomEngine<std::mt19937_64>&& random_engine)
                : color_(color),
                  metalness_(std::uniform_real_distribution<T>(0, 1)(random_engine)),
                  roughness_(std::uniform_real_distribution<T>(min_roughness, 1)(random_engine))
        {
        }

public:
        TestBRDF(const Color& color, const std::type_identity_t<T>& min_roughness)
                : TestBRDF(color, min_roughness, create_engine<std::mt19937_64>())
        {
        }

        const Color& color() const
        {
                return color_;
        }
};

template <std::size_t N, typename T, typename Color>
void test_brdf_white(const unsigned sample_count)
{
        const TestBRDF<N, T, Color> brdf(Color(1), MIN_ROUGHNESS<T>);

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", uniform, white");
        {
                const Color color = directional_albedo_uniform_sampling(brdf, sample_count);
                check_color_less(color, brdf.color());
        }

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", importance, white");
        {
                const Color color = directional_albedo_importance_sampling(brdf, sample_count);
                check_color_less(color, brdf.color());
        }
}

template <std::size_t N, typename T, typename Color>
void test_brdf_random(const unsigned sample_count)
{
        const TestBRDF<N, T, Color> brdf(random_non_black_color<Color>(), MIN_ROUGHNESS<T>);

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", uniform, random");
        {
                const Color color = directional_albedo_uniform_sampling(brdf, sample_count);
                check_color_range(color);
        }

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", importance, random");
        {
                const Color color = directional_albedo_importance_sampling(brdf, sample_count);
                check_color_range(color);
        }
}

template <std::size_t N, typename T, typename Color>
void test_brdf_pdf(const unsigned sample_count)
{
        const TestBRDF<N, T, Color> brdf(Color(1), MIN_ROUGHNESS<T>);

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", PDF integral");
        const T integral = directional_pdf_integral(brdf, sample_count);
        if (!(std::abs(integral - 1) <= T(0.05)))
        {
                error("BRDF error, PDF integral is not equal to 1\n" + to_string(integral));
        }
}

template <std::size_t N, typename T, typename Color, typename Counter>
void test_brdf(const Counter& counter)
{
        constexpr unsigned SAMPLE_COUNT = 1'000'000;

        counter();
        test_brdf_white<N, T, Color>(SAMPLE_COUNT);
        counter();
        test_brdf_random<N, T, Color>(SAMPLE_COUNT);
        counter();
        test_brdf_pdf<N, T, Color>(2 * SAMPLE_COUNT);
}

void test_small(ProgressRatio* const progress)
{
        LOG("Test GGX Diffuse BRDF");

        constexpr int COUNT = 3 * 3;
        int i = -1;
        const auto counter = [&]
        {
                progress->set(++i, COUNT);
        };

        test_brdf<3, double, color::Color>(counter);
        test_brdf<4, double, color::Color>(counter);
        test_brdf<5, double, color::Color>(counter);

        LOG("Test GGX Diffuse BRDF passed");
}

void test_large(ProgressRatio* const progress)
{
        LOG("Test GGX Diffuse BRDF");

        constexpr int COUNT = 3 * 12;
        int i = -1;
        const auto counter = [&]
        {
                progress->set(++i, COUNT);
        };

        test_brdf<3, float, color::Color>(counter);
        test_brdf<4, float, color::Color>(counter);
        test_brdf<5, float, color::Color>(counter);

        test_brdf<3, double, color::Color>(counter);
        test_brdf<4, double, color::Color>(counter);
        test_brdf<5, double, color::Color>(counter);

        test_brdf<3, float, color::Spectrum>(counter);
        test_brdf<4, float, color::Spectrum>(counter);
        test_brdf<5, float, color::Spectrum>(counter);

        test_brdf<3, double, color::Spectrum>(counter);
        test_brdf<4, double, color::Spectrum>(counter);
        test_brdf<5, double, color::Spectrum>(counter);

        LOG("Test GGX Diffuse BRDF passed");
}
}

TEST_SMALL("BRDF, GGX Diffuse", test_small)
TEST_LARGE("BRDF, GGX Diffuse", test_large)
}
