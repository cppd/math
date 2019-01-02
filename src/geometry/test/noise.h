/*
Copyright (C) 2017-2019 Topological Manifold

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

#if 0

#include "com/error.h"
#include "com/random/engine.h"
#include "com/vec.h"

#include <random>
#include <vector>

template <size_t N, typename T>
void add_noise(std::vector<Vector<N, T>>* points, T delta)
{
        static_assert(!std::is_integral_v<T>);

        // std::mt19937_64 engine(points.size());
        RandomEngineWithSeed<std::mt19937_64> engine;

        std::uniform_real_distribution<T> urd(-1.0, 1.0);

        for (size_t i = 0; i < points->size(); ++i)
        {
                Vector<N, T> r;
                do
                {
                        for (unsigned n = 0; n < N; ++n)
                        {
                                r[n] = urd(engine);
                        }
                } while (dot(r, r) > 1);

                (*points)[i] = (*points)[i] + delta * r;
        }
}

template <size_t N, typename T>
void add_discrete_noise(std::vector<Vector<N, T>>* points, T delta, int size)
{
        static_assert(!std::is_integral_v<T>);

        if (size < 1)
        {
                error("discrete noise size < 1");
        }

        // std::mt19937_64 engine(points.size());
        RandomEngineWithSeed<std::mt19937_64> engine;

        std::uniform_int_distribution<int> urd(-size, size);
        double sqr = square(size);

        for (size_t i = 0; i < points->size(); ++i)
        {
                Vector<N, T> r;
                do
                {
                        for (unsigned n = 0; n < N; ++n)
                        {
                                r[n] = urd(engine);
                        }
                } while (dot(r, r) > sqr);

                (*points)[i] = (*points)[i] + delta / size * r;
        }
}

#endif
