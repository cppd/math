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

constexpr unsigned SAMPLE_COUNT = 1'000'000;

template <typename T>
constexpr T MIN_ROUGHNESS = 0.2;

template <std::size_t N, typename T, typename Color>
class BRDF final : public TestBRDF<N, T, Color, RandomEngine<T>>
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

public:
        BRDF(const Color& color, std::type_identity_t<T> min_roughness)
        {
                color_ = color;
                RandomEngine<std::mt19937_64> random_engine = create_engine<std::mt19937_64>();
                roughness_ = std::uniform_real_distribution<T>(min_roughness, 1)(random_engine);
                metalness_ = std::uniform_real_distribution<T>(0, 1)(random_engine);
        }

        const Color& color() const
        {
                return color_;
        }
};

template <std::size_t N, typename T, typename Color>
void test_brdf_white()
{
        const BRDF<N, T, Color> brdf(Color(1), MIN_ROUGHNESS<T>);

        Color result;

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", f, white");
        result = test_brdf_f(brdf, SAMPLE_COUNT);
        check_color_less(result, brdf.color());

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", sample f, white");
        result = test_brdf_sample_f(brdf, SAMPLE_COUNT);
        check_color_less(result, brdf.color());
}

template <std::size_t N, typename T, typename Color>
void test_brdf_random()
{
        const BRDF<N, T, Color> brdf(random_non_black_color<Color>(), MIN_ROUGHNESS<T>);

        Color result;

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", f, random");
        result = test_brdf_f(brdf, SAMPLE_COUNT);
        check_color_range(result);

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", sample f, random");
        result = test_brdf_sample_f(brdf, SAMPLE_COUNT);
        check_color_range(result);
}

template <std::size_t N, typename T, typename Color>
void test_brdf_pdf()
{
        const BRDF<N, T, Color> brdf(Color(1), MIN_ROUGHNESS<T>);

        LOG(std::string(Color::name()) + ", " + to_string(N) + "D, " + type_name<T>() + ", f, pdf");
        T pdf = test_brdf_pdf(brdf, SAMPLE_COUNT);
        if (!(pdf > 0 && pdf < T(1.01)))
        {
                error("BRDF error, PDF is not less than 1\n" + to_string(pdf));
        }
}

template <typename T, typename Color>
void test_brdf(ProgressRatio* const progress)
{
        progress->set(0.0 / 9);
        test_brdf_white<3, T, Color>();

        progress->set(1.0 / 9);
        test_brdf_random<3, T, Color>();

        progress->set(2.0 / 9);
        test_brdf_pdf<3, T, Color>();

        //

        progress->set(3.0 / 9);
        test_brdf_white<4, T, Color>();

        progress->set(4.0 / 9);
        test_brdf_random<4, T, Color>();

        progress->set(5.0 / 9);
        test_brdf_pdf<4, T, Color>();

        //

        progress->set(6.0 / 9);
        test_brdf_white<5, T, Color>();

        progress->set(7.0 / 9);
        test_brdf_random<5, T, Color>();

        progress->set(8.0 / 9);
        test_brdf_pdf<5, T, Color>();
}

void test_small(ProgressRatio* const progress)
{
        LOG("Test GGX Diffuse BRDF");

        test_brdf<double, color::Color>(progress);

        LOG("Test GGX Diffuse BRDF passed");
}

void test_large(ProgressRatio* const progress)
{
        LOG("Test GGX Diffuse BRDF");

        progress->set_text("float Color");
        test_brdf<float, color::Color>(progress);

        progress->set_text("double Color");
        test_brdf<double, color::Color>(progress);

        progress->set_text("float Spectrum");
        test_brdf<float, color::Spectrum>(progress);

        progress->set_text("double Spectrum");
        test_brdf<double, color::Spectrum>(progress);

        LOG("Test GGX Diffuse BRDF passed");
}
}

TEST_SMALL("BRDF, GGX Diffuse", test_small)
TEST_LARGE("BRDF, GGX Diffuse", test_large)
}
