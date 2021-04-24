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

#include "../../objects.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/sampling/sphere_uniform.h>

namespace ns::painter::brdf::test
{
namespace brdf_implementation
{
template <std::size_t N, typename T>
Vector<N, T> random_v(RandomEngine<T>& random_engine, const Vector<N, T>& n)
{
        Vector<N, T> v;
        do
        {
                v = sampling::uniform_on_sphere<N, T>(random_engine);
        } while (dot(n, v) <= 0);
        return v;
}
}

template <std::size_t N, typename T>
class TestBRDF
{
protected:
        ~TestBRDF() = default;

public:
        virtual Color f(const Vector<N, T>& n, const Vector<N, T>& v, const Vector<N, T>& l) const = 0;

        virtual BrdfSample<N, T> sample_f(RandomEngine<T>& random_engine, const Vector<N, T>& n, const Vector<N, T>& v)
                const = 0;
};

template <std::size_t N, typename T>
Color test_brdf_f(const TestBRDF<N, T>& brdf, const unsigned sample_count)
{
        const T UNIFORM_ON_HEMISPHERE_PDF = 2 * sampling::uniform_on_sphere_pdf<N, T>();

        RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();

        const Vector<N, T> n = sampling::uniform_on_sphere<N, T>(random_engine);
        const Vector<N, T> v = brdf_implementation::random_v(random_engine, n);

        Color sum(0);
        unsigned sample = 0;

        while (sample < sample_count)
        {
                const Vector<N, T> l = sampling::uniform_on_sphere<N, T>(random_engine);
                const T n_l = dot(n, l);

                if (n_l <= 0)
                {
                        Color c = brdf.f(n, v, l);
                        if (!c.is_black())
                        {
                                error("BRDF color " + to_string(c.rgb<float>()) + " is not black when dot(n,l) <= 0");
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

template <std::size_t N, typename T>
Color test_brdf_sample_f(const TestBRDF<N, T>& brdf, const unsigned sample_count)
{
        RandomEngine<T> random_engine = create_engine<RandomEngine<T>>();

        const Vector<N, T> n = sampling::uniform_on_sphere<N, T>(random_engine);
        const Vector<N, T> v = brdf_implementation::random_v(random_engine, n);

        Color sum(0);

        for (unsigned i = 0; i < sample_count; ++i)
        {
                BrdfSample<N, T> sample = brdf.sample_f(random_engine, n, v);
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
