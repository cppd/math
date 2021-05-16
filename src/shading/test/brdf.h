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

#pragma once

#include "../sample.h"

#include <src/com/error.h>
#include <src/com/random/engine.h>
#include <src/sampling/sphere_uniform.h>

#include <array>
#include <random>

namespace ns::shading::test
{
namespace brdf_implementation
{
template <std::size_t N, typename T>
std::array<Vector<N, T>, 2> random_n_v()
{
        std::mt19937 random_engine = create_engine<std::mt19937>();

        Vector<N, T> n = sampling::uniform_on_sphere<N, T>(random_engine);
        Vector<N, T> v;
        T d;
        do
        {
                v = sampling::uniform_on_sphere<N, T>(random_engine);
                d = dot(n, v);
        } while (!(d != 0));
        if (d > 0)
        {
                return {n, v};
        }
        return {n, -v};
}
}

template <std::size_t N, typename T, typename RandomEngine>
class TestBRDF
{
protected:
        ~TestBRDF() = default;

public:
        virtual Color f(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const = 0;

        virtual Sample<N, T> sample_f(RandomEngine& random_engine, const Vector<N, T>& n, const Vector<N, T>& v)
                const = 0;
};

template <std::size_t N, typename T, typename RandomEngine>
Color test_brdf_f(const TestBRDF<N, T, RandomEngine>& brdf, const long long sample_count)
{
        if (sample_count <= 0)
        {
                error("Sample count must be positive");
        }

        const T UNIFORM_ON_HEMISPHERE_PDF = 2 * sampling::uniform_on_sphere_pdf<N, T>();

        const auto [n, v] = brdf_implementation::random_n_v<N, T>();

        RandomEngine random_engine = create_engine<RandomEngine>();

        Color sum(0);
        long long sample = 0;

        while (sample < sample_count)
        {
                const Vector<N, T> l = sampling::uniform_on_sphere<N, T>(random_engine);
                const T n_l = dot(n, l);

                if (n_l <= 0)
                {
                        Color c = brdf.f(n, v, l);
                        if (!c.is_black())
                        {
                                error("BRDF color is not black when dot(n,l) <= 0 " + to_string(c));
                        }
                        continue;
                }

                ++sample;
                Color c = brdf.f(n, v, l);
                if (c.is_black())
                {
                        continue;
                }
                sum += c * (n_l / UNIFORM_ON_HEMISPHERE_PDF);
        }

        return sum / sample_count;
}

template <std::size_t N, typename T, typename RandomEngine>
Color test_brdf_sample_f(const TestBRDF<N, T, RandomEngine>& brdf, const long long sample_count)
{
        if (sample_count <= 0)
        {
                error("Sample count must be positive");
        }

        const auto [n, v] = brdf_implementation::random_n_v<N, T>();

        RandomEngine random_engine = create_engine<RandomEngine>();

        Color sum(0);

        for (long long i = 0; i < sample_count; ++i)
        {
                Sample<N, T> sample = brdf.sample_f(random_engine, n, v);
                if (sample.brdf.is_black() || sample.pdf <= 0)
                {
                        continue;
                }
                const T n_l = dot(n, sample.l);
                if (n_l <= 0)
                {
                        continue;
                }
                sum += sample.brdf * (n_l / sample.pdf);
        }

        return sum / sample_count;
}
}
