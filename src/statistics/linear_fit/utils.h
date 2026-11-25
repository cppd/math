/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/vector.h>
#include <src/statistics/median.h>

#include <cstddef>
#include <tuple>
#include <vector>

namespace ns::statistics::linear_fit::utils
{
template <typename T>
void check_breakpoints(const std::vector<T>& breakpoints)
{
        if (breakpoints.size() < 2)
        {
                error("Breakpoints size (" + to_string(breakpoints.size()) + ") is less than 2");
        }

        for (std::size_t i = 1; i < breakpoints.size(); ++i)
        {
                if (!(breakpoints[i] > breakpoints[i - 1]))
                {
                        error("Breakpoints are not strictly ordered");
                }
        }
}

template <typename T>
void check_point_count(const std::vector<numerical::Vector<2, T>>& points)
{
        const std::size_t n = points.size();
        if (n < 2)
        {
                error("Points count (" + to_string(n) + ") is less than 2");
        }
}

template <typename T>
std::vector<T> compute_all_slopes(const std::vector<numerical::Vector<2, T>>& points)
{
        const std::size_t n = points.size();

        std::vector<T> res;

        res.reserve(n * (n + 1) / 2);
        for (std::size_t i = 0; i < n; ++i)
        {
                for (std::size_t j = i + 1; j < n; ++j)
                {
                        const T dx = points[j][0] - points[i][0];
                        if (dx != 0)
                        {
                                const T dy = points[j][1] - points[i][1];
                                res.push_back(dy / dx);
                        }
                }
        }

        return res;
}

template <typename T>
std::vector<T> compute_slope_medians(const std::vector<numerical::Vector<2, T>>& points)
{
        const std::size_t n = points.size();

        std::vector<T> data;
        data.reserve(n);

        std::vector<T> res;
        res.reserve(n);

        for (std::size_t i = 0; i < n; ++i)
        {
                data.clear();
                for (std::size_t j = 0; j < n; ++j)
                {
                        const T dx = points[j][0] - points[i][0];
                        if (dx != 0)
                        {
                                const T dy = points[j][1] - points[i][1];
                                data.push_back(dy / dx);
                        }
                }
                res.push_back(median(&data));
        }

        return res;
}

template <typename T>
std::tuple<T, T> intercept_slope(const std::vector<numerical::Vector<2, T>>& points, std::vector<T>* const slopes)
{
        ASSERT(slopes);

        const T slope = median(slopes);

        slopes->clear();
        for (const numerical::Vector<2, T>& p : points)
        {
                slopes->push_back(p[1] - slope * p[0]);
        }

        const T intercept = median(slopes);

        return {intercept, slope};
}

template <typename T>
std::vector<std::vector<T>> least_squares_a(const std::vector<numerical::Vector<2, T>>& points)
{
        std::vector<std::vector<T>> a(2);

        a[0].resize(points.size(), 1);
        a[1].reserve(points.size());

        for (const numerical::Vector<2, T>& p : points)
        {
                a[1].push_back(p[0]);
        }

        return a;
}

template <typename T>
std::vector<std::vector<T>> least_squares_a(
        const std::vector<numerical::Vector<2, T>>& points,
        const std::vector<T>& breakpoints)
{
        std::vector<std::vector<T>> a(1 + breakpoints.size());

        a[0].resize(points.size(), 1);
        for (std::size_t i = 1; i < a.size(); ++i)
        {
                a[i].reserve(points.size());
        }

        for (const numerical::Vector<2, T>& p : points)
        {
                const T x = p[0];
                for (std::size_t i = 0; i < breakpoints.size(); ++i)
                {
                        const T b = breakpoints[i];
                        a[i + 1].push_back(x > b ? x - b : 0);
                }
        }

        return a;
}

template <typename T>
std::vector<T> least_squares_b(const std::vector<numerical::Vector<2, T>>& points)
{
        std::vector<T> b;
        b.reserve(points.size());
        for (const numerical::Vector<2, T>& p : points)
        {
                b.push_back(p[1]);
        }
        return b;
}
}
