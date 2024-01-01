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

#pragma once

#include "../objects.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/numerical/vector.h>
#include <src/sampling/sphere_uniform.h>

#include <cstddef>

namespace ns::shading::compute
{
template <std::size_t N, typename T, typename Color>
class BRDF
{
protected:
        ~BRDF() = default;

public:
        [[nodiscard]] virtual Color f(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const = 0;

        [[nodiscard]] virtual T pdf(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const = 0;

        [[nodiscard]] virtual Sample<N, T, Color> sample_f(PCG& engine, const Vector<N, T>& n, const Vector<N, T>& v)
                const = 0;
};

template <std::size_t N, typename T, typename Color, typename RandomEngine>
Color directional_albedo_uniform_sampling(
        const BRDF<N, T, Color>& brdf,
        const Vector<N, T>& n,
        const Vector<N, T>& v,
        const long long sample_count,
        RandomEngine& engine)
{
        if (sample_count <= 0)
        {
                error("Sample count " + to_string(sample_count) + " must be positive");
        }

        static constexpr T UNIFORM_ON_HEMISPHERE_PDF = sampling::uniform_on_hemisphere_pdf<N, T>();

        Color sum{0};

        long long i = 0;
        while (i < sample_count)
        {
                const Vector<N, T> l = sampling::uniform_on_sphere<N, T>(engine);
                const T n_l = dot(n, l);

                if (n_l <= 0)
                {
                        const Color c = brdf.f(n, v, l);
                        if (!c.is_black())
                        {
                                error("BRDF color is not black when dot(n, l) <= 0 " + to_string(c));
                        }
                        continue;
                }

                ++i;
                sum += brdf.f(n, v, l) * (n_l / UNIFORM_ON_HEMISPHERE_PDF);
        }

        return sum / sample_count;
}

template <std::size_t N, typename T, typename Color, typename RandomEngine>
Color directional_albedo_importance_sampling(
        const BRDF<N, T, Color>& brdf,
        const Vector<N, T>& n,
        const Vector<N, T>& v,
        const long long sample_count,
        RandomEngine& engine)
{
        if (sample_count <= 0)
        {
                error("Sample count " + to_string(sample_count) + " must be positive");
        }

        Color sum{0};

        long long i = 0;
        while (i < sample_count)
        {
                const Sample<N, T, Color> sample = brdf.sample_f(engine, n, v);

                if (!std::isfinite(sample.pdf))
                {
                        error("Sample PDF " + to_string(sample.pdf) + " is not finite");
                }

                if (sample.pdf <= 0)
                {
                        continue;
                }

                const T n_l = dot(n, sample.l);
                if (n_l <= 0)
                {
                        if (!sample.brdf.is_black())
                        {
                                error("BRDF color is not black when dot(n, l) <= 0 " + to_string(sample.brdf));
                        }
                        continue;
                }

                ++i;
                sum += sample.brdf * (n_l / sample.pdf);
        }

        return sum / sample_count;
}

template <std::size_t N, typename T, typename Color, typename RandomEngine>
T directional_pdf_integral(
        const BRDF<N, T, Color>& brdf,
        const Vector<N, T>& n,
        const Vector<N, T>& v,
        const long long sample_count,
        RandomEngine& engine)
{
        if (sample_count <= 0)
        {
                error("Sample count " + to_string(sample_count) + " must be positive");
        }

        T sum{0};

        for (long long i = 0; i < sample_count; ++i)
        {
                const Vector<N, T> l = sampling::uniform_on_sphere<N, T>(engine);
                const T pdf = brdf.pdf(n, v, l);
                if (!std::isfinite(pdf))
                {
                        error("Sample PDF " + to_string(pdf) + " is not finite");
                }
                sum += pdf;
        }

        return sum / (sample_count * sampling::uniform_on_sphere_pdf<N, T>());
}
}
