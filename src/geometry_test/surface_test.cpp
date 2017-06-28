/*
Copyright (C) 2017 Topological Manifold

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

#include "surface_test.h"

#include "com/log.h"
#include "com/math.h"
#include "com/print.h"
#include "com/random.h"
#include "geometry_cocone/surface.h"
#include "progress/progress.h"

#include <cmath>
#include <random>

namespace
{
template <size_t N, typename T>
void add_noise(std::vector<Vector<N, T>>* points, T delta)
{
        static_assert(!std::is_integral<T>::value);

        // std::mt19937_64 engine(points.size());
        std::mt19937_64 engine(get_random_seed<std::mt19937_64>());

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
        static_assert(!std::is_integral<T>::value);

        if (size < 1)
        {
                error("discrete noise size < 1");
        }

        // std::mt19937_64 engine(points.size());
        std::mt19937_64 engine(get_random_seed<std::mt19937_64>());

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

#if 0
void generate_random_data(int cnt, std::vector<Vector<2, float>>* source_points)
{
        source_points->clear();

        for (double i = 0; i < cnt; ++i)
        {
                Vector<2, float> v;
                v[0] = 0 + 1.1 * std::cos(2 * PI * i / cnt);
                v[1] = std::sin(2 * PI * i / cnt);
                source_points->push_back(v);
        }

        for (double i = 0; i < cnt; ++i)
        {
                Vector<2, float> v;
                v[0] = 0 + 1.2 * std::cos(2 * PI * i / cnt);
                v[1] = std::sin(2 * PI * i / cnt);
                source_points->push_back(v);
        }

        //

        for (double i = 0; i < cnt; ++i)
        {
                Vector<2, float> v;
                v[0] = 10 + 1.1 * std::cos(2 * PI * i / cnt);
                v[1] = 0.5 * std::sin(2 * PI * i / cnt);
                source_points->push_back(v);
        }

        for (double i = 0; i < cnt; ++i)
        {
                Vector<2, float> v;
                v[0] = 10 + 1.2 * std::cos(2 * PI * i / cnt);
                v[1] = 0.5 * std::sin(2 * PI * i / cnt);
                source_points->push_back(v);
        }

        //

        for (double i = 0; i < cnt; ++i)
        {
                Vector<2, float> v;
                v[0] = 100 + 1.1 * std::cos(2 * PI * i / cnt);
                v[1] = std::sin(2 * PI * i / cnt);
                source_points->push_back(v);
        }

        for (double i = 0; i < cnt; ++i)
        {
                Vector<2, float> v;
                v[0] = 100 + 1.2 * std::cos(2 * PI * i / cnt);
                v[1] = std::sin(2 * PI * i / cnt);
                source_points->push_back(v);
        }
}
#else
#if 1
template <size_t N>
void generate_random_data(int count, std::vector<Vector<N, float>>* points)
{
        std::mt19937_64 gen(count);
        std::uniform_real_distribution<double> urd(-1.0, 1.0);

        points->resize(count);

        for (int v_i = 0; v_i < count; ++v_i)
        {
                vec<N> v;
                do
                {
                        for (unsigned i = 0; i < N; ++i)
                        {
                                v[i] = urd(gen);
                        }
                } while (length(v) > 1);

                (*points)[v_i] = to_vec<float>(normalize(v));
                (*points)[v_i][0] *= 2;
        }
}
#else
void generate_random_data(int count, std::vector<Vector<2, float>>* points)
{
        points->resize(count);

        for (double v_i = 0; v_i < count; ++v_i)
        {
                (*points)[v_i] = {-std::cos(PI * v_i / (count - 1)), std::sin(PI * v_i / (count - 1))};
        }
}
#endif
#endif
}

void surface_test()
{
        try
        {
                LOG("-----------------");

                constexpr int N = 3;

                constexpr int count = 1000;
                std::vector<Vector<N, float>> points;
                std::vector<Vector<N, double>> normals;
                std::vector<std::array<int, N>> facets;

                ProgressRatio progress(nullptr);

                generate_random_data(count, &points);

                std::unique_ptr<ISurfaceReconstructor<3>> sr = create_surface_reconstructor(points, &progress);

                sr->cocone(&normals, &facets, &progress);
                sr->bound_cocone(0.3, 0.14, &normals, &facets, &progress);
                // sr->bound_cocone(0.3, 0.14, &normals, &facets, &progress);

                LOG("object count: " + to_string(facets.size()));

                LOG("");

                // std::exit(EXIT_SUCCESS);
        }
        catch (std::exception& e)
        {
                error_fatal(std::string("surface test error: ") + e.what());
        }
}
