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
#include <src/shading/lambertian/brdf.h>
#include <src/shading/objects.h>
#include <src/shading/testing/color.h>
#include <src/shading/testing/random.h>
#include <src/test/test.h>

#include <cstddef>
#include <string>

namespace ns::shading::lambertian
{
namespace
{
template <std::size_t N, typename T, typename Color>
class TestBRDF final : public compute::BRDF<N, T, Color>
{
        const Color color_;

public:
        template <typename RandomEngine>
        explicit TestBRDF(RandomEngine& engine)
                : color_(testing::random_non_black_color<Color>(engine))
        {
        }

        [[nodiscard]] Color f(
                const numerical::Vector<N, T>& n,
                const numerical::Vector<N, T>& v,
                const numerical::Vector<N, T>& l) const override
        {
                if (dot(n, v) <= 0)
                {
                        return Color(0);
                }
                return lambertian::f(color_, n, l);
        }

        [[nodiscard]] T pdf(
                const numerical::Vector<N, T>& n,
                const numerical::Vector<N, T>& v,
                const numerical::Vector<N, T>& l) const override
        {
                if (dot(n, v) <= 0)
                {
                        return 0;
                }
                return lambertian::pdf(n, l);
        }

        [[nodiscard]] Sample<N, T, Color> sample_f(
                PCG& engine,
                const numerical::Vector<N, T>& n,
                const numerical::Vector<N, T>& v) const override
        {
                if (dot(n, v) <= 0)
                {
                        return {numerical::Vector<N, T>(0), 0, Color(0)};
                }
                return lambertian::sample_f(engine, color_, n);
        }

        [[nodiscard]] const Color& color() const
        {
                return color_;
        }

        [[nodiscard]] std::string description() const
        {
                return space_name(N);
        }
};

template <std::size_t N, typename T, typename Color, typename RandomEngine>
void test_brdf(RandomEngine& engine)
{
        constexpr unsigned SAMPLE_COUNT = 100'000;

        const TestBRDF<N, T, Color> brdf(engine);

        const auto [n, v] = testing::random_n_v<N, T>(engine);

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", uniform");
        const Color color_uniform = compute::directional_albedo_uniform_sampling(brdf, n, v, SAMPLE_COUNT, engine);
        testing::check_color_equal(color_uniform, brdf.color());

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", importance");
        const Color color_importance =
                compute::directional_albedo_importance_sampling(brdf, n, v, SAMPLE_COUNT, engine);
        testing::check_color_equal(color_importance, brdf.color());

        constexpr double RELATIVE_ERROR = 0.01;
        testing::check_uniform_importance_equal(
                color_uniform, color_importance, RELATIVE_ERROR,
                [&]
                {
                        return brdf.description();
                });

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", PDF integral");
        const T integral = compute::directional_pdf_integral(brdf, n, v, SAMPLE_COUNT, engine);
        if (!(std::abs(integral - 1) <= T{0.02}))
        {
                error("BRDF error, PDF integral is not equal to 1\n" + to_string(integral));
        }
}

template <typename T, typename Color, typename Counter, typename RandomEngine>
void test_brdf(const Counter& counter, RandomEngine& engine)
{
        counter();
        test_brdf<3, T, Color>(engine);
        counter();
        test_brdf<4, T, Color>(engine);
        counter();
        test_brdf<5, T, Color>(engine);
}

template <typename Color, typename Counter, typename RandomEngine>
void test_brdf(const Counter& counter, RandomEngine& engine)
{
        test_brdf<float, Color>(counter, engine);
        test_brdf<double, Color>(counter, engine);
}

void test(progress::Ratio* const progress)
{
        LOG("Test Lambertian BRDF");

        PCG engine;

        constexpr int COUNT = 3 * 2 * 2;
        int i = -1;
        const auto counter = [&]
        {
                progress->set(++i, COUNT);
        };

        test_brdf<color::Color>(counter, engine);
        test_brdf<color::Spectrum>(counter, engine);

        LOG("Test Lambertian BRDF passed");
}

//

template <std::size_t N, typename T, typename Color>
void test_distribution(
        const TestBRDF<N, T, Color>& brdf,
        const numerical::Vector<N, T>& n,
        const numerical::Vector<N, T>& v,
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
                [&](const numerical::Vector<N, T>& l)
                {
                        return brdf.pdf(n, v, l);
                },
                progress);
}

template <std::size_t N, typename T, typename Color, typename RandomEngine>
void test_sampling(progress::Ratio* const progress, RandomEngine& engine)
{
        LOG("Lambertian Sampling, " + space_name(N) + ", " + type_name<T>());

        const TestBRDF<N, T, Color> brdf(engine);
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

TEST_SMALL("BRDF, Lambertian", test)
TEST_LARGE("BRDF, Lambertian Sampling, 3-space", test_sampling<3>)
TEST_LARGE("BRDF, Lambertian Sampling, 4-space", test_sampling<4>)
TEST_LARGE("BRDF, Lambertian Sampling, 5-space", test_sampling<5>)
}
