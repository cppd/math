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

#include "median.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/qr.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <vector>

namespace ns::statistics
{
namespace linear_fit_implementation
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
}

template <typename T>
struct Line final
{
        T intercept;
        T slope;
};

template <typename T>
[[nodiscard]] Line<T> linear_fit_qr(const std::vector<numerical::Vector<2, T>>& points)
{
        static_assert(std::is_floating_point_v<T>);

        std::vector<std::vector<T>> a(2);
        std::vector<T> b;

        a[0].reserve(points.size());
        a[1].reserve(points.size());
        b.reserve(points.size());

        for (const numerical::Vector<2, T>& p : points)
        {
                a[0].push_back(1);
                a[1].push_back(p[0]);
                b.push_back(p[1]);
        }

        const std::vector<T> s = numerical::qr_solve(a, b);
        ASSERT(s.size() == 2);

        return {
                .intercept = s[0],
                .slope = s[1],
        };
}

template <typename T>
[[nodiscard]] Line<T> linear_fit_theil_sen(const std::vector<numerical::Vector<2, T>>& points)
{
        const std::size_t n = points.size();

        if (n < 2)
        {
                error("Points count (" + to_string(n) + ") is less than 2");
        }

        std::vector<T> data;

        data.reserve(n * (n + 1) / 2);
        for (std::size_t i = 0; i < n; ++i)
        {
                for (std::size_t j = i + 1; j < n; ++j)
                {
                        const T dx = points[j][0] - points[i][0];
                        if (dx == 0)
                        {
                                continue;
                        }
                        const T dy = points[j][1] - points[i][1];
                        data.push_back(dy / dx);
                }
        }

        const T slope = median(&data);

        data.clear();
        data.reserve(n);
        for (const numerical::Vector<2, T>& p : points)
        {
                data.push_back(p[1] - slope * p[0]);
        }

        const T intercept = median(&data);

        return {
                .intercept = intercept,
                .slope = slope,
        };
}

template <typename T>
[[nodiscard]] std::vector<T> linear_fit_piecewise(
        const std::vector<numerical::Vector<2, T>>& points,
        const std::vector<T>& breakpoints)
{
        linear_fit_implementation::check_breakpoints(breakpoints);

        std::vector<std::vector<T>> a(1 + breakpoints.size());
        std::vector<T> b;

        for (std::vector<T>& v : a)
        {
                v.reserve(points.size());
        }
        b.reserve(points.size());

        for (const numerical::Vector<2, T>& p : points)
        {
                a[0].push_back(1);
                for (std::size_t i = 0; i < breakpoints.size(); ++i)
                {
                        a[i + 1].push_back(p[0] > breakpoints[i] ? p[0] - breakpoints[i] : 0);
                }
                b.push_back(p[1]);
        }

        std::vector<T> s = numerical::qr_solve(a, b);
        ASSERT(s.size() == breakpoints.size() + 1);

        return s;
}
}
