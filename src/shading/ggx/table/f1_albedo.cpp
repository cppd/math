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

#include "f1_albedo.h"

#include "../brdf.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/thread.h>
#include <src/geometry/shapes/sphere_integral.h>
#include <src/numerical/interpolation.h>
#include <src/numerical/vector.h>
#include <src/settings/dimensions.h>
#include <src/shading/compute/brdf.h>
#include <src/shading/objects.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <ios>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace ns::shading::ggx::table
{
namespace
{
using ComputeType = double;
constexpr std::size_t SIZE = 32;
constexpr int SAMPLE_COUNT = 100'000'000;

constexpr int PRECISION = 6;
constexpr int ROW_SIZE = 8;
constexpr std::string_view INDENT = "        ";

static_assert(INDENT.size() == 8);

constexpr std::string_view ALBEDO_NAME = "F1_ALBEDO_COSINE_ROUGHNESS";
constexpr std::string_view ALBEDO_COSINE_NAME = "F1_ALBEDO_COSINE_WEIGHTED_AVERAGE";

template <std::size_t N, typename T, typename Color>
class ComputeBRDF final : public compute::BRDF<N, T, Color>
{
        static constexpr bool GGX_ONLY = true;
        static constexpr Color F0 = Color(1);
        static constexpr Color RHO_SS = Color(0);
        static constexpr Colors<Color> COLORS{F0, RHO_SS};

        T roughness_ = 1;

public:
        [[nodiscard]] Color f(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
        {
                return brdf::f<GGX_ONLY>(roughness_, COLORS, n, v, l);
        }

        [[nodiscard]] T pdf(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
        {
                return brdf::pdf<GGX_ONLY>(roughness_, n, v, l);
        }

        [[nodiscard]] Sample<N, T, Color> sample_f(PCG& engine, const Vector<N, T>& n, const Vector<N, T>& v)
                const override
        {
                return brdf::sample_f<GGX_ONLY>(engine, roughness_, COLORS, n, v);
        }

        void set_roughness(const T roughness)
        {
                roughness_ = roughness;
        }
};

template <typename Color>
[[nodiscard]] float scalar_albedo(const Color& color_albedo)
{
        const Vector<3, float> rgb = color_albedo.rgb32();
        ASSERT(rgb[0] == rgb[1] && rgb[1] == rgb[2]);

        const float r = rgb[0];

        if (!(r >= 0))
        {
                error("Albedo " + to_string(r) + " is not non-negative");
        }

        if (!(r < 1.01f))
        {
                error("Albedo " + to_string(r) + " is greater than 1");
        }

        return std::min(r, 1.0f);
}

template <std::size_t N, typename T, std::size_t COUNT, typename Color, typename RandomEngine>
void compute(
        const std::size_t roughness_index,
        const std::size_t cosine_index,
        const int sample_count,
        const Vector<N, T>& n,
        Vector<N, T>* const v,
        ComputeBRDF<N, T, Color>* const brdf,
        std::array<std::array<T, COUNT>, COUNT>* const data,
        RandomEngine& engine)
{
        static_assert(N >= 2);

        const T roughness = static_cast<T>(roughness_index == 0 ? T{0.01} : roughness_index) / (COUNT - 1);
        const T cosine = static_cast<T>(cosine_index == 0 ? T{0.01} : cosine_index) / (COUNT - 1);
        const T sine = std::sqrt(1 - square(cosine));

        ASSERT(roughness > 0 && roughness <= 1);
        ASSERT(cosine > 0 && cosine <= 1);
        ASSERT(sine >= 0 && sine <= 1);

        brdf->set_roughness(roughness);

        (*v)[N - 1] = cosine;
        (*v)[N - 2] = sine;

        const Color color_albedo =
                (roughness_index == 0 && cosine_index != 0)
                        ? Color(1)
                        : directional_albedo_importance_sampling(*brdf, n, *v, sample_count, engine);

        (*data)[roughness_index][cosine_index] = scalar_albedo(color_albedo);
}

template <std::size_t N, typename T, std::size_t COUNT>
std::array<std::array<T, COUNT>, COUNT> compute_albedo()
{
        using Color = color::RGB<T>;

        const Vector<N, T> n = []
        {
                Vector<N, T> res(0);
                res[N - 1] = 1;
                return res;
        }();

        std::array<std::array<T, COUNT>, COUNT> data;

        constexpr std::size_t TASK_COUNT = COUNT * COUNT;

        run_in_threads(
                [&](std::atomic_size_t& task)
                {
                        ComputeBRDF<N, T, Color> brdf;
                        Vector<N, T> v(0);
                        PCG engine;

                        std::size_t index = 0;
                        while ((index = task++) < TASK_COUNT)
                        {
                                const std::size_t roughness_i = index / COUNT;
                                const std::size_t cosine_i = index % COUNT;

                                compute(roughness_i, cosine_i, SAMPLE_COUNT, n, &v, &brdf, &data, engine);

                                LOG(to_string(N) + "D (" + to_string(roughness_i) + "," + to_string(cosine_i) + ") "
                                    + to_string(data[roughness_i][cosine_i]));
                        }
                },
                TASK_COUNT);

        return data;
}

template <std::size_t N, typename T, std::size_t COUNT>
std::array<T, COUNT> compute_cosine_weighted_average(const std::array<std::array<T, COUNT>, COUNT>& data)
{
        std::array<T, COUNT> res;
        for (std::size_t roughness_i = 0; roughness_i < COUNT; ++roughness_i)
        {
                numerical::Interpolation<1, T, T> interpolation({COUNT}, data[roughness_i]);

                constexpr int AVERAGE_COUNT = 1000;
                const T average = geometry::shapes::sphere_cosine_weighted_average_by_cosine<N, T>(
                        [&](const T cosine)
                        {
                                return interpolation.compute(Vector<1, T>(cosine));
                        },
                        AVERAGE_COUNT);

                if (!(average >= 0))
                {
                        error("Cosine-weighted average " + to_string(average) + " is not non-negative");
                }

                if (!(average < T{1.01}))
                {
                        error("Cosine-weighted average " + to_string(average) + " is greater than 1");
                }

                res[roughness_i] = std::min(average, T{1});
        }
        return res;
}

template <std::size_t N, typename T, std::size_t COUNT>
void write_albedo(const std::array<std::array<T, COUNT>, COUNT>& data, std::ostringstream& oss)
{
        oss << "template <typename T>\n";
        oss << "constexpr std::array " << ALBEDO_NAME;
        oss << "<" + to_string(N) + ", T> = std::to_array<T>\n";
        oss << "({\n";
        oss << INDENT;
        for (std::size_t roughness_i = 0, i = 0; roughness_i < COUNT; ++roughness_i)
        {
                for (std::size_t angle_i = 0; angle_i < COUNT; ++angle_i, ++i)
                {
                        if (i > 0)
                        {
                                oss << ",";
                                if (i % ROW_SIZE == 0)
                                {
                                        oss << "\n" << INDENT;
                                }
                                else
                                {
                                        oss << " ";
                                }
                        }
                        oss << data[roughness_i][angle_i];
                }
        }
        oss << "\n});\n";
}

template <std::size_t N, typename T, std::size_t COUNT>
void write_cosine_weighted_average(const std::array<T, COUNT>& data, std::ostringstream& oss)
{
        oss << "template <typename T>\n";
        oss << "constexpr std::array " << ALBEDO_COSINE_NAME;
        oss << "<" + to_string(N) + ", T> = std::to_array<T>\n";
        oss << "({\n";
        oss << INDENT;
        for (std::size_t roughness_i = 0; roughness_i < COUNT; ++roughness_i)
        {
                if (roughness_i > 0)
                {
                        oss << ",";
                        if (roughness_i % ROW_SIZE == 0)
                        {
                                oss << "\n" << INDENT;
                        }
                        else
                        {
                                oss << " ";
                        }
                }
                oss << data[roughness_i];
        }
        oss << "\n});\n";
}

template <std::size_t N>
void f1_albedo(std::ostringstream& oss)
{
        static_assert(N >= 2);

        const auto albedo = compute_albedo<N, ComputeType, SIZE>();
        const auto cosine_weighted_average = compute_cosine_weighted_average<N, ComputeType>(albedo);

        write_albedo<N>(albedo, oss);
        oss << "\n";
        write_cosine_weighted_average<N>(cosine_weighted_average, oss);
}

template <std::size_t... I>
std::string f1_albedo_tables(std::index_sequence<I...>&&)
{
        std::ostringstream oss;
        oss << std::setprecision(PRECISION) << std::fixed;

        oss << "// clang-format off\n";
        oss << "\n";

        oss << "constexpr std::array<int, 2> " << ALBEDO_NAME << "_SIZE = {" << SIZE << ", " << SIZE << "};\n";
        oss << "constexpr std::array<int, 1> " << ALBEDO_COSINE_NAME << "_SIZE = {" << SIZE << "};\n";
        oss << "\n";
        oss << "template <std::size_t N, typename T>\n";
        oss << "constexpr std::array<T, 0> " << ALBEDO_NAME << ";\n";
        oss << "\n";
        oss << "template <std::size_t N, typename T>\n";
        oss << "constexpr std::array<T, 0> " << ALBEDO_COSINE_NAME << ";\n";
        oss << "\n";

        ((f1_albedo<I>(oss), (oss << "\n")), ...);

        oss << "// clang-format on\n";

        return oss.str();
}
}

std::string f1_albedo_tables()
{
        return f1_albedo_tables(settings::Dimensions());
}
}
