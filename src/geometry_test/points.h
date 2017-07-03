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

#pragma once

#include "noise.h"

#include "com/error.h"
#include "com/log.h"
#include "com/math.h"
#include "geometry/vec.h"

#include <random>
#include <unordered_set>
#include <vector>

namespace PointsImplementation
{
template <size_t N>
void check_unique_points(const std::vector<Vector<N, float>>& points)
{
        std::unordered_set<Vector<N, float>> check_set(points.cbegin(), points.cend());

        if (points.size() != check_set.size())
        {
                error("error generate unique points");
        }
}
}

inline std::vector<Vector<2, float>> generate_points_semicircle(int point_count)
{
        std::vector<Vector<2, float>> points(point_count);

        for (double i = 0; i < point_count; ++i)
        {
                points[i] = {-std::cos(PI * i / (point_count - 1)), std::sin(PI * i / (point_count - 1))};
        }

        PointsImplementation::check_unique_points(points);

        return points;
}

template <size_t N, size_t Discretization>
std::vector<Vector<N, float>> generate_points_ellipsoid(int point_count)
{
        std::vector<Vector<N, float>> points(point_count);

        constexpr int D = Discretization;

        std::mt19937_64 gen(point_count);
        std::uniform_int_distribution<int> uid(-D, D);

        for (int i = 0; i < point_count; ++i)
        {
                vec<N> v;
                do
                {
                        for (unsigned n = 0; n < N; ++n)
                        {
                                v[n] = static_cast<double>(uid(gen)) / D;
                        }
                } while (dot(v, v) > 1);

                v = normalize(v);

                v[0] *= 2;

                points[i] = to_vec<float>(v);
        }

        PointsImplementation::check_unique_points(points);

        return points;
}

template <size_t N, size_t Discretization>
std::vector<Vector<N, float>> generate_points_object_recess(unsigned point_count)
{
        std::vector<Vector<N, float>> points(point_count);

        constexpr int D = Discretization;

        std::mt19937_64 gen(point_count);
        std::uniform_int_distribution<int> uid(-D, D);

        Vector<N, double> z_axis(0);
        z_axis[N - 1] = 1;

        for (unsigned i = 0; i < point_count; ++i)
        {
                // точки на сфере с углублением со стороны последней оси
                // в положительном направлении этой оси

                vec<N> v;
                do
                {
                        for (unsigned n = 0; n < N; ++n)
                        {
                                v[n] = static_cast<double>(uid(gen)) / D;
                        }
                } while (dot(v, v) > 1);

                v = normalize(v);

                double dot_z = dot(z_axis, v);
                if (dot_z > 0)
                {
                        v[N - 1] *= 1 - std::abs(0.3 * std::pow(dot_z, 10));
                }

                points[i] = to_vec<float>(v);
        }

        PointsImplementation::check_unique_points(points);

        return points;
}
