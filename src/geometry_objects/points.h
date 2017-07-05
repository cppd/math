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

template <size_t N>
Vector<N, long> to_integer(const Vector<N, double>& v, long factor)
{
        Vector<N, long> r;
        for (unsigned n = 0; n < N; ++n)
        {
                r[n] = std::lround(v[n] * factor);
        }
        return r;
}

template <size_t N>
vec<N> random_sphere(std::mt19937_64* gen)
{
        std::uniform_real_distribution<double> urd(-1.0, 1.0);

        vec<N> v;
        do
        {
                for (unsigned n = 0; n < N; ++n)
                {
                        v[n] = urd(*gen);
                }
        } while (dot(v, v) > 1);

        return normalize(v);
}
}

inline std::vector<Vector<2, float>> generate_points_semicircle(unsigned point_count)
{
        if (point_count < 3)
        {
                error("point count out of range");
        }

        std::vector<Vector<2, float>> points(point_count);

        for (double i = 0; i < point_count; ++i)
        {
                points[i] = {-std::cos(PI * i / (point_count - 1)), std::sin(PI * i / (point_count - 1))};
        }

        PointsImplementation::check_unique_points(points);

        return points;
}

template <size_t N>
std::vector<Vector<N, float>> generate_points_ellipsoid(unsigned point_count, unsigned discretization)
{
        // Для float большое число не надо
        if (discretization > 1000000)
        {
                error("discretization out of range");
        }

        std::vector<Vector<N, float>> points;
        points.reserve(point_count);

        std::unordered_set<Vector<N, long>> integer_points;
        integer_points.reserve(point_count);

        std::mt19937_64 gen(point_count);

        while (integer_points.size() < point_count)
        {
                vec<N> v = PointsImplementation::random_sphere<N>(&gen);

                v[0] *= 2;

                Vector<N, long> integer_point = PointsImplementation::to_integer(v, discretization);
                if (integer_points.count(integer_point) == 0)
                {
                        integer_points.insert(integer_point);
                        points.push_back(to_vector<float>(v));
                }
        }

        PointsImplementation::check_unique_points(points);

        return points;
}

template <size_t N>
std::vector<Vector<N, float>> generate_points_object_recess(unsigned point_count, unsigned discretization)
{
        // Для float большое число не надо
        if (discretization > 1000000)
        {
                error("discretization out of range");
        }

        std::vector<Vector<N, float>> points;
        points.reserve(point_count);

        std::unordered_set<Vector<N, long>> integer_points;
        integer_points.reserve(point_count);

        std::mt19937_64 gen(point_count);

        Vector<N, double> z_axis(0);
        z_axis[N - 1] = 1;

        while (integer_points.size() < point_count)
        {
                // точки на сфере с углублением со стороны последней оси
                // в положительном направлении этой оси

                vec<N> v = PointsImplementation::random_sphere<N>(&gen);

                double dot_z = dot(z_axis, v);
                if (dot_z > 0)
                {
                        v[N - 1] *= 1 - std::abs(0.3 * std::pow(dot_z, 10));
                }

                Vector<N, long> integer_point = PointsImplementation::to_integer(v, discretization);
                if (integer_points.count(integer_point) == 0)
                {
                        integer_points.insert(integer_point);
                        points.push_back(to_vector<float>(v));
                }
        }

        PointsImplementation::check_unique_points(points);

        return points;
}
