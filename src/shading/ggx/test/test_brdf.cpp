/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "../brdf.h"
#include "../metalness.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/numerical/vector.h>
#include <src/progress/progress.h>
#include <src/sampling/testing/test.h>
#include <src/shading/compute/brdf.h>
#include <src/shading/objects.h>
#include <src/shading/testing/color.h>
#include <src/shading/testing/random.h>
#include <src/test/test.h>

#include <cstddef>
#include <random>
#include <string>

namespace ns::shading::ggx
{
namespace
{
template <typename T>
constexpr T MIN_ROUGHNESS = 0.35;

template <std::size_t N, typename T, typename Color>
class TestBRDF final : public compute::BRDF<N, T, Color>
{
        Color color_;
        Colors<Color> colors_;
        T roughness_;

public:
        template <typename RandomEngine>
        TestBRDF(const Color& color, RandomEngine& engine)
                : color_(color),
                  colors_(compute_metalness(color, std::uniform_real_distribution<T>(0, 1)(engine))),
                  roughness_(std::uniform_real_distribution<T>(MIN_ROUGHNESS<T>, 1)(engine))
        {
        }

        [[nodiscard]] Color f(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
        {
                return brdf::f(roughness_, colors_, n, v, l);
        }

        [[nodiscard]] T pdf(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
        {
                return brdf::pdf(roughness_, n, v, l);
        }

        [[nodiscard]] Sample<N, T, Color> sample_f(PCG& engine, const Vector<N, T>& n, const Vector<N, T>& v)
                const override
        {
                return brdf::sample_f(engine, roughness_, colors_, n, v);
        }

        [[nodiscard]] const Color& color() const
        {
                return color_;
        }

        [[nodiscard]] std::string description() const
        {
                return space_name(N) + ", roughness = " + to_string(roughness_);
        }
};

template <std::size_t N, typename T, typename Color, typename RandomEngine>
void test_brdf_white(const unsigned sample_count, RandomEngine& engine)
{
        const TestBRDF<N, T, Color> brdf(Color(1), engine);

        const auto [n, v] = testing::random_n_v<N, T>(engine);

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", uniform, white");
        const Color color_uniform = compute::directional_albedo_uniform_sampling(brdf, n, v, sample_count, engine);
        testing::check_color_less(color_uniform, brdf.color());

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", importance, white");
        const Color color_importance =
                compute::directional_albedo_importance_sampling(brdf, n, v, sample_count, engine);
        testing::check_color_less(color_importance, brdf.color());

        constexpr double RELATIVE_ERROR = 0.25;

        testing::check_uniform_importance_equal(
                color_uniform, color_importance, RELATIVE_ERROR,
                [&]
                {
                        return brdf.description();
                });
}

template <std::size_t N, typename T, typename Color, typename RandomEngine>
void test_brdf_random(const unsigned sample_count, RandomEngine& engine)
{
        const TestBRDF<N, T, Color> brdf(testing::random_non_black_color<Color>(engine), engine);

        const auto [n, v] = testing::random_n_v<N, T>(engine);

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", uniform, random");
        const Color color_uniform = compute::directional_albedo_uniform_sampling(brdf, n, v, sample_count, engine);
        testing::check_color_range(color_uniform);

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", importance, random");
        const Color color_importance =
                compute::directional_albedo_importance_sampling(brdf, n, v, sample_count, engine);
        testing::check_color_range(color_importance);

        constexpr double RELATIVE_ERROR = 0.25;

        testing::check_uniform_importance_equal(
                color_uniform, color_importance, RELATIVE_ERROR,
                [&]
                {
                        return brdf.description();
                });
}

template <std::size_t N, typename T, typename Color, typename RandomEngine>
void test_brdf_pdf(const unsigned sample_count, RandomEngine& engine)
{
        const TestBRDF<N, T, Color> brdf(Color(1), engine);

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", PDF integral");
        {
                const auto [n, v] = testing::random_n_v<N, T>(engine);
                const T integral = compute::directional_pdf_integral(brdf, n, v, sample_count, engine);
                if (!(std::abs(integral - 1) <= T{0.05}))
                {
                        error("BRDF error, PDF integral is not equal to 1\n" + to_string(integral));
                }
        }
}

template <std::size_t N, typename T, typename Color, typename Counter, typename RandomEngine>
void test_brdf(const Counter& counter, RandomEngine& engine)
{
        constexpr unsigned SAMPLE_COUNT = 1'000'000;

        counter();
        test_brdf_white<N, T, Color>(SAMPLE_COUNT, engine);
        counter();
        test_brdf_random<N, T, Color>(SAMPLE_COUNT, engine);
        counter();
        test_brdf_pdf<N, T, Color>(2 * SAMPLE_COUNT, engine);
}

template <typename T, typename Color, typename Counter, typename RandomEngine>
void test_brdf(const Counter& counter, RandomEngine& engine)
{
        test_brdf<3, T, Color>(counter, engine);
        test_brdf<4, T, Color>(counter, engine);
        test_brdf<5, T, Color>(counter, engine);
}

void test_small(progress::Ratio* const progress)
{
        LOG("Test GGX Diffuse BRDF");

        PCG engine;

        constexpr int COUNT = 3 * 3;
        int i = -1;
        const auto counter = [&]
        {
                progress->set(++i, COUNT);
        };

        test_brdf<double, color::Color>(counter, engine);

        LOG("Test GGX Diffuse BRDF passed");
}

void test_large(progress::Ratio* const progress)
{
        LOG("Test GGX Diffuse BRDF");

        PCG engine;

        constexpr int COUNT = 3 * 3 * 4;
        int i = -1;
        const auto counter = [&]
        {
                progress->set(++i, COUNT);
        };

        test_brdf<float, color::Color>(counter, engine);
        test_brdf<double, color::Color>(counter, engine);
        test_brdf<float, color::Spectrum>(counter, engine);
        test_brdf<double, color::Spectrum>(counter, engine);

        LOG("Test GGX Diffuse BRDF passed");
}

//

template <std::size_t N, typename T, typename Color>
void test_distribution(
        const TestBRDF<N, T, Color>& brdf,
        const Vector<N, T>& n,
        const Vector<N, T>& v,
        progress::Ratio* const progress)
{
        constexpr int COUNT_PER_BUCKET = 10'000;

        sampling::testing::test_distribution_surface<N, T>(
                "", COUNT_PER_BUCKET,
                [&](auto& engine)
                {
                        for (int i = 0; i < 10; ++i)
                        {
                                const Sample<N, T, Color> sample = brdf.sample_f(engine, n, v);
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
                [&](const Vector<N, T>& l)
                {
                        return brdf.pdf(n, v, l);
                },
                progress);
}

template <std::size_t N, typename T, typename Color, typename RandomEngine>
void test_sampling(progress::Ratio* const progress, RandomEngine& engine)
{
        LOG("GGX Diffuse Sampling, " + space_name(N) + ", " + type_name<T>());

        const TestBRDF<N, T, Color> brdf(Color(1), engine);
        const auto [n, v] = testing::random_n_v<N, T>(engine);

        test_distribution(brdf, n, v, progress);
}

template <std::size_t N>
void test_sampling(progress::Ratio* const progress)
{
        using Color = color::Spectrum;

        PCG engine;

        test_sampling<N, float, Color>(progress, engine);
        test_sampling<N, double, Color>(progress, engine);
}
}

TEST_SMALL("BRDF, GGX Diffuse", test_small)
TEST_LARGE("BRDF, GGX Diffuse", test_large)
TEST_LARGE("BRDF, GGX Diffuse Sampling, 3-space", test_sampling<3>)
TEST_LARGE("BRDF, GGX Diffuse Sampling, 4-space", test_sampling<4>)
TEST_LARGE("BRDF, GGX Diffuse Sampling, 5-space", test_sampling<5>)
}
