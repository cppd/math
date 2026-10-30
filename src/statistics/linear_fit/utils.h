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
#include <src/numerical/qr.h>
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
std::tuple<T, T> median_mad(std::vector<T> data)
{
        const T m = median(&data);
        for (T& v : data)
        {
                v = std::abs(v - m);
        }
        const T mad = median(&data);
        return {m, mad};
}

template <typename T>
std::vector<T> residuals(const std::vector<numerical::Vector<2, T>>& points, const T intercept, const T slope)
{
        std::vector<T> res;
        res.reserve(points.size());
        for (const numerical::Vector<2, T>& p : points)
        {
                res.push_back(intercept + slope * p[0] - p[1]);
        }
        return res;
}

template <typename T>
std::vector<numerical::Vector<2, T>> filter_points(
        const std::vector<numerical::Vector<2, T>>& points,
        const std::vector<T>& filter,
        const T gate)
{
        ASSERT(points.size() == filter.size());

        const auto [median, mad] = median_mad(filter);
        const T min = median - gate * mad;
        const T max = median + gate * mad;

        std::vector<numerical::Vector<2, T>> res;
        res.reserve(points.size());
        for (std::size_t i = 0; i < points.size(); ++i)
        {
                if (filter[i] >= min && filter[i] <= max)
                {
                        res.push_back(points[i]);
                }
        }
        return res;
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

template <typename T>
std::vector<std::vector<numerical::Vector<2, T>>> breakpoint_segments(
        const std::vector<numerical::Vector<2, T>>& points,
        const std::vector<T>& breakpoints)
{
        std::vector<std::vector<numerical::Vector<2, T>>> res(breakpoints.size());

        for (const numerical::Vector<2, T>& p : points)
        {
                for (auto i = std::ssize(breakpoints) - 1; i >= 0; --i)
                {
                        if (p[0] >= breakpoints[i])
                        {
                                res[i].push_back(p);
                                break;
                        }
                }
        }

        return res;
}
}
