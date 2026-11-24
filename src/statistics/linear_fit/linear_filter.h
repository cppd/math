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

#include <src/numerical/vector.h>

#include <vector>

namespace ns::statistics::linear_fit
{
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
