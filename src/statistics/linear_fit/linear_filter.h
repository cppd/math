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

#include "linear_fit.h"
#include "utils.h"

#include <src/com/error.h>
#include <src/numerical/vector.h>
#include <src/statistics/median.h>

#include <cstddef>
#include <tuple>
#include <vector>

namespace ns::statistics::linear_fit
{
namespace linear_filter_implementation
{
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

template <typename T>
std::vector<numerical::Vector<2, T>> filter_points(
        const std::vector<numerical::Vector<2, T>>& points,
        const std::vector<T>& residuals,
        const T gate)
{
        ASSERT(points.size() == residuals.size());

        const auto [median, mad] = median_mad(residuals);
        const T min = median - gate * mad;
        const T max = median + gate * mad;

        std::vector<numerical::Vector<2, T>> res;
        res.reserve(points.size());
        for (std::size_t i = 0; i < points.size(); ++i)
        {
                if (residuals[i] >= min && residuals[i] <= max)
                {
                        res.push_back(points[i]);
                }
        }
        return res;
}
}

template <typename T>
[[nodiscard]] std::vector<numerical::Vector<2, T>> filter_points(
        const std::vector<numerical::Vector<2, T>>& points,
        const T gate)
{
        static_assert(std::is_floating_point_v<T>);

        namespace impl = linear_filter_implementation;

        utils::check_point_count(points);

        const Line l = linear_fit_medians(points);
        const std::vector<T> residuals = impl::residuals(points, l.intercept, l.slope);
        return impl::filter_points(points, residuals, gate);
}

template <typename T>
[[nodiscard]] std::vector<numerical::Vector<2, T>> filter_points(
        const std::vector<numerical::Vector<2, T>>& points,
        const std::vector<T>& breakpoints,
        const T gate)
{
        static_assert(std::is_floating_point_v<T>);

        namespace impl = linear_filter_implementation;

        utils::check_breakpoints(breakpoints);

        std::vector<numerical::Vector<2, T>> res;
        for (const auto& segment_points : impl::breakpoint_segments(points, breakpoints))
        {
                res.insert_range(res.end(), filter_points(segment_points, gate));
        }
        return res;
}
}
