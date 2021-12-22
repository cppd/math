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

#include "color.h"
#include "random.h"

#include "../compute/brdf.h"
#include "../ggx_diffuse.h"

#include <src/color/color.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/name.h>
#include <src/sampling/testing/test.h>
#include <src/test/test.h>

#include <random>

namespace ns::shading::test
{
namespace
{
template <typename T>
using RandomEngine = std::conditional_t<sizeof(T) <= 4, std::mt19937, std::mt19937_64>;

template <typename T>
constexpr T MIN_ROUGHNESS = 0.35;

template <std::size_t N, typename T, typename Color>
class TestBRDF final : public compute::BRDF<N, T, Color, RandomEngine<T>>
{
        Color color_;
        T metalness_;
        T roughness_;

        TestBRDF(const Color& color, RandomEngine<std::mt19937_64>&& random_engine)
                : color_(color),
                  metalness_(std::uniform_real_distribution<T>(0, 1)(random_engine)),
                  roughness_(std::uniform_real_distribution<T>(MIN_ROUGHNESS<T>, 1)(random_engine))
        {
        }

public:
        explicit TestBRDF(const Color& color) : TestBRDF(color, create_engine<std::mt19937_64>())
        {
        }

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

        const Color& color() const
        {
                return color_;
        }
};

template <std::size_t N, typename T, typename Color>
void test_brdf_white(const unsigned sample_count)
{
        const TestBRDF<N, T, Color> brdf(Color(1));

        const auto [n, v] = random_n_v<N, T>();

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", uniform, white");
        const Color color_uniform = compute::directional_albedo_uniform_sampling(brdf, n, v, sample_count);
        check_color_less(color_uniform, brdf.color());

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", importance, white");
        const Color color_importance = compute::directional_albedo_importance_sampling(brdf, n, v, sample_count);
        check_color_less(color_importance, brdf.color());

        constexpr double RELATIVE_ERROR = 0.25;
        check_uniform_importance_equal(color_uniform, color_importance, RELATIVE_ERROR);
}

template <std::size_t N, typename T, typename Color>
void test_brdf_random(const unsigned sample_count)
{
        const TestBRDF<N, T, Color> brdf(random_non_black_color<Color>());

        const auto [n, v] = random_n_v<N, T>();

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", uniform, random");
        const Color color_uniform = compute::directional_albedo_uniform_sampling(brdf, n, v, sample_count);
        check_color_range(color_uniform);

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", importance, random");
        const Color color_importance = compute::directional_albedo_importance_sampling(brdf, n, v, sample_count);
        check_color_range(color_importance);

        constexpr double RELATIVE_ERROR = 0.25;
        check_uniform_importance_equal(color_uniform, color_importance, RELATIVE_ERROR);
}

template <std::size_t N, typename T, typename Color>
void test_brdf_pdf(const unsigned sample_count)
{
        const TestBRDF<N, T, Color> brdf(Color(1));

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", PDF integral");
        {
                const auto [n, v] = random_n_v<N, T>();
                const T integral = compute::directional_pdf_integral(brdf, n, v, sample_count);
                if (!(std::abs(integral - 1) <= T(0.05)))
                {
                        error("BRDF error, PDF integral is not equal to 1\n" + to_string(integral));
                }
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

template <typename T, typename Color, typename Counter>
void test_brdf(const Counter& counter)
{
        test_brdf<3, T, Color>(counter);
        test_brdf<4, T, Color>(counter);
        test_brdf<5, T, Color>(counter);
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

        test_brdf<double, color::Color>(counter);

        LOG("Test GGX Diffuse BRDF passed");
}

void test_large(ProgressRatio* const progress)
{
        LOG("Test GGX Diffuse BRDF");

        constexpr int COUNT = 3 * 3 * 4;
        int i = -1;
        const auto counter = [&]
        {
                progress->set(++i, COUNT);
        };

        test_brdf<float, color::Color>(counter);
        test_brdf<double, color::Color>(counter);
        test_brdf<float, color::Spectrum>(counter);
        test_brdf<double, color::Spectrum>(counter);

        LOG("Test GGX Diffuse BRDF passed");
}

//

template <std::size_t N, typename T, typename Color>
void test_sampling(ProgressRatio* const progress)
{
        constexpr int COUNT_PER_BUCKET = 10'000;

        LOG("GGX Diffuse Sampling, " + space_name(N) + ", " + type_name<T>());

        const TestBRDF<N, T, Color> brdf(Color(1));
        const auto [n, v] = random_n_v<N, T>();

        sampling::testing::test_distribution_surface<N, T, RandomEngine<T>>(
                "", COUNT_PER_BUCKET,
                [&, v = v, n = n](RandomEngine<T>& random_engine)
                {
                        for (int i = 0; i < 10; ++i)
                        {
                                const Sample<N, T, Color> sample = brdf.sample_f(random_engine, n, v);
                                if (!(sample.pdf >= 0))
                                {
                                        error("Sample PDF " + to_string(sample.pdf) + " is not non-negative");
                                }
                                if (sample.pdf > 0)
                                {
                                        return sample.l;
                                }
                        }
                        error("No positive PDF found");
                },
                [&, v = v, n = n](const Vector<N, T>& l)
                {
                        return brdf.pdf(n, v, l);
                },
                progress);
}

template <std::size_t N>
void test_sampling(ProgressRatio* const progress)
{
        using Color = color::Spectrum;
        test_sampling<N, float, Color>(progress);
        test_sampling<N, double, Color>(progress);
}
}

TEST_SMALL("BRDF, GGX Diffuse", test_small)
TEST_LARGE("BRDF, GGX Diffuse", test_large)
TEST_LARGE("BRDF, GGX Diffuse Sampling, 3-space", test_sampling<3>)
TEST_LARGE("BRDF, GGX Diffuse Sampling, 4-space", test_sampling<4>)
TEST_LARGE("BRDF, GGX Diffuse Sampling, 5-space", test_sampling<5>)
}
