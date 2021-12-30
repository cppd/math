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
}

template <std::size_t N>
std::string ggx_reflection()
{
        static_assert(N >= 2);

        using T = double;
        using Color = color::RGB<T>;

        constexpr int COUNT = 32;
        constexpr int SAMPLE_COUNT = 1'000'000;

        const Vector<N, T> n = []
        {
                Vector<N, T> res(0);
                res[N - 1] = 1;
                return res;
        }();

        ComputeBRDF<N, T, Color> brdf;

        PCG engine;

        Vector<N, T> v(0);

        std::ostringstream oss_albedo;
        oss_albedo << std::setprecision(4) << std::scientific;

        std::ostringstream oss_average;
        oss_average << std::setprecision(4) << std::scientific;
        oss_average << "cosine-weighted average {";

        for (int roughness_i = 0; roughness_i < COUNT; ++roughness_i)
        {
                const T roughness = static_cast<T>(roughness_i) / (COUNT - 1);

                oss_albedo << "roughness [" << roughness << "]: {";
                brdf.set_roughness(roughness);

                T sum = 0;
                T cosine_sum = 0;
                for (int angle_i = 0; angle_i < COUNT; ++angle_i)
                {
                        const T cosine = static_cast<T>(angle_i) / (COUNT - 1);

                        v[N - 1] = cosine;
                        v[N - 2] = std::sqrt(1 - square(cosine));

                        const Color color_albedo = [&]
                        {
                                if (roughness == 0 || cosine == 0)
                                {
                                        return Color(1);
                                }
                                return directional_albedo_importance_sampling(brdf, n, v, SAMPLE_COUNT, engine);
                        }();
                        const Vector<3, float> rgb = color_albedo.rgb32();
                        ASSERT(rgb[0] == rgb[1] && rgb[1] == rgb[2]);

                        const T albedo = rgb[0];

                        if (angle_i > 0)
                        {
                                oss_albedo << ", ";
                        }
                        oss_albedo << albedo;

                        LOG("(" + to_string(roughness_i) + "," + to_string(angle_i) + ") " + to_string(albedo));

                        sum += cosine * albedo;
                        cosine_sum += cosine;
                }
                oss_albedo << "}\n";

                if (roughness_i > 0)
                {
                        oss_average << ", ";
                }
                oss_average << sum / cosine_sum;
        }
        oss_average << "}";

        return oss_albedo.str() + oss_average.str();
}

template std::string ggx_reflection<3>();
}
