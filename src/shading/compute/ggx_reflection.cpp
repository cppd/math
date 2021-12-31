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

#include "ggx_reflection.h"

#include "brdf.h"

#include "../ggx_diffuse.h"

#include <src/color/color.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/thread.h>

#include <cmath>
#include <sstream>

namespace ns::shading::compute
{
namespace
{
template <std::size_t N, typename T, typename Color>
class ComputeBRDF final : public BRDF<N, T, Color>
{
        static constexpr bool GGX_ONLY = true;

        Colors<Color> colors_{Color(1), Color(0)};
        T roughness_ = 1;

public:
        Color f(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
        {
                return ggx_diffuse::f<GGX_ONLY>(roughness_, colors_, n, v, l);
        }

        T pdf(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const override
        {
                return ggx_diffuse::pdf<GGX_ONLY>(roughness_, n, v, l);
        }

        Sample<N, T, Color> sample_f(PCG& engine, const Vector<N, T>& n, const Vector<N, T>& v) const override
        {
                return ggx_diffuse::sample_f<GGX_ONLY>(engine, roughness_, colors_, n, v);
        }

        void set_roughness(const T roughness)
        {
                roughness_ = roughness;
        }
};

template <typename T>
struct Albedo
{
        T albedo;
        T cosine_albedo;
};

template <std::size_t N, typename T, std::size_t COUNT, typename Color, typename RandomEngine>
void compute(
        const std::size_t roughness_index,
        const std::size_t cosine_index,
        const int sample_count,
        const Vector<N, T>& n,
        Vector<N, T>* const v,
        ComputeBRDF<N, T, Color>* const brdf,
        std::array<std::array<Albedo<T>, COUNT>, COUNT>* const data,
        RandomEngine& engine)
{
        const T roughness = static_cast<T>(roughness_index == 0 ? T(0.01) : roughness_index) / (COUNT - 1);
        const T cosine = static_cast<T>(cosine_index == 0 ? T(0.01) : cosine_index) / (COUNT - 1);
        ASSERT(roughness >= 0 && roughness <= 1);
        ASSERT(cosine >= 0 && cosine <= 1);

        brdf->set_roughness(roughness);

        (*v)[N - 1] = cosine;
        (*v)[N - 2] = std::sqrt(1 - square(cosine));

        const Color color_albedo = [&]
        {
                if (roughness == 0 || cosine == 0)
                {
                        return Color(1);
                }
                return directional_albedo_importance_sampling(*brdf, n, *v, sample_count, engine);
        }();

        const auto albedo = [&]
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

                return std::min<decltype(r)>(r, 1);
        }();

        (*data)[roughness_index][cosine_index].albedo = albedo;
        (*data)[roughness_index][cosine_index].cosine_albedo = albedo * cosine;
}

template <std::size_t N, typename T, std::size_t COUNT>
std::array<std::array<Albedo<T>, COUNT>, COUNT> compute_albedo()
{
        using Color = color::RGB<T>;

        constexpr int SAMPLE_COUNT = 50'000'000;

        const Vector<N, T> n = []
        {
                Vector<N, T> res(0);
                res[N - 1] = 1;
                return res;
        }();

        std::array<std::array<Albedo<T>, COUNT>, COUNT> data;

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

                                LOG("(" + to_string(roughness_i) + "," + to_string(cosine_i) + ") "
                                    + to_string(data[roughness_i][cosine_i].albedo));
                        }
                },
                TASK_COUNT);

        return data;
}

template <typename T, std::size_t COUNT>
std::array<T, COUNT> compute_cosine_weighted_average(const std::array<std::array<Albedo<T>, COUNT>, COUNT>& data)
{
        std::array<T, COUNT> res;
        for (std::size_t roughness_i = 0; roughness_i < COUNT; ++roughness_i)
        {
                long double sum = 0;
                for (std::size_t cosine_i = 0; cosine_i < COUNT; ++cosine_i)
                {
                        sum += data[roughness_i][cosine_i].cosine_albedo;
                }

                const T average = sum / 0.5 * COUNT; // cosine sum == (0.0 + 1.0) / 2 * COUNT == 0.5 * COUNT

                if (!(average >= 0))
                {
                        error("Cosine-weighted average " + to_string(average) + " is not non-negative");
                }

                if (!(average < T(1.01)))
                {
                        error("Cosine-weighted average " + to_string(average) + " is greater than 1");
                }

                res[roughness_i] = std::min<decltype(average)>(average, 1);
        }
        return res;
}

template <typename T, std::size_t COUNT>
std::string albedo_to_string(const std::array<std::array<Albedo<T>, COUNT>, COUNT>& data)
{
        std::ostringstream oss;
        oss << std::setprecision(5) << std::scientific;

        oss << "{\n";
        for (std::size_t roughness_i = 0; roughness_i < COUNT; ++roughness_i)
        {
                if (roughness_i > 0)
                {
                        oss << ",\n";
                }
                oss << "{";
                for (std::size_t angle_i = 0; angle_i < COUNT; ++angle_i)
                {
                        if (angle_i > 0)
                        {
                                oss << ", ";
                        }
                        oss << data[roughness_i][angle_i].albedo;
                }
                oss << "}";
        }
        oss << "\n}\n";

        return oss.str();
}

template <typename T, std::size_t COUNT>
std::string cosine_weighted_average_to_string(const std::array<T, COUNT>& data)
{
        std::ostringstream oss;
        oss << std::setprecision(5) << std::scientific;

        oss << "{";
        for (std::size_t roughness_i = 0; roughness_i < COUNT; ++roughness_i)
        {
                if (roughness_i > 0)
                {
                        oss << ", ";
                }
                oss << data[roughness_i];
        }
        oss << "}";

        return oss.str();
}
}

template <std::size_t N>
std::string ggx_reflection()
{
        static_assert(N >= 2);

        using T = double;

        constexpr std::size_t COUNT = 32;

        const auto albedo = compute_albedo<N, T, COUNT>();
        const auto cosine_weighted_average = compute_cosine_weighted_average(albedo);

        return albedo_to_string(albedo) + "\n" + cosine_weighted_average_to_string(cosine_weighted_average);
}

template std::string ggx_reflection<3>();
}
