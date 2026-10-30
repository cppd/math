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

#include "utils.h"

#include <src/com/error.h>
#include <src/numerical/qr.h>
#include <src/numerical/vector.h>

#include <vector>

namespace ns::statistics::linear_fit
{
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

        std::vector<std::vector<T>> a = utils::least_squares_a(points);
        std::vector<T> b = utils::least_squares_b(points);

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
        static_assert(std::is_floating_point_v<T>);

        utils::check_point_count(points);

        std::vector<T> slopes = utils::compute_all_slopes(points);

        const auto [intercept, slope] = utils::intercept_slope(points, &slopes);

        return {
                .intercept = intercept,
                .slope = slope,
        };
}

template <typename T>
[[nodiscard]] Line<T> linear_fit_medians(const std::vector<numerical::Vector<2, T>>& points)
{
        static_assert(std::is_floating_point_v<T>);

        utils::check_point_count(points);

        std::vector<T> slopes = utils::compute_slope_medians(points);
        ASSERT(slopes.size() == points.size());

        const auto [intercept, slope] = utils::intercept_slope(points, &slopes);

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
        static_assert(std::is_floating_point_v<T>);

        utils::check_breakpoints(breakpoints);

        std::vector<std::vector<T>> a = utils::least_squares_a(points, breakpoints);
        std::vector<T> b = utils::least_squares_b(points);

        std::vector<T> s = numerical::qr_solve(a, b);
        ASSERT(s.size() == breakpoints.size() + 1);

        return s;
}

template <typename T>
[[nodiscard]] std::vector<numerical::Vector<2, T>> filter_points(
        const std::vector<numerical::Vector<2, T>>& points,
        const T gate)
{
        static_assert(std::is_floating_point_v<T>);

        utils::check_point_count(points);

        const Line l = linear_fit_medians(points);

        const std::vector<T> residuals = utils::residuals(points, l.intercept, l.slope);

        return utils::filter_points(points, residuals, gate);
}

template <typename T>
[[nodiscard]] std::vector<numerical::Vector<2, T>> filter_points(
        const std::vector<numerical::Vector<2, T>>& points,
        const std::vector<T>& breakpoints,
        const T gate)
{
        static_assert(std::is_floating_point_v<T>);

        utils::check_breakpoints(breakpoints);

        std::vector<numerical::Vector<2, T>> res;
        for (const auto& segment_points : utils::breakpoint_segments(points, breakpoints))
        {
                res.insert_range(res.end(), filter_points(segment_points, gate));
        }
        return res;
}
}
