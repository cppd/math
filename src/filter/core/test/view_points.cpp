/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "view_points.h"

#include "time_update_info.h"

#include "view/point.h"

#include <src/com/error.h>
#include <src/filter/core/smooth.h>
#include <src/numerical/matrix_object.h>
#include <src/numerical/vector_object.h>

#include <cstddef>
#include <vector>

namespace ns::filter::core::test
{
template <typename T>
std::vector<view::Point<T>> view_points(const std::vector<TimeUpdateInfo<T>>& result)
{
        std::vector<view::Point<T>> res;
        res.reserve(result.size());
        for (const TimeUpdateInfo<T>& r : result)
        {
                res.push_back(
                        {.time = r.time,
                         .x = r.info.x,
                         .x_stddev = r.info.x_stddev,
                         .v = r.info.v,
                         .v_stddev = r.info.v_stddev});
        }
        return res;
}

template <typename T>
std::vector<view::Point<T>> smooth_view_points(const std::vector<TimeUpdateInfo<T>>& result)
{
        if (result.empty())
        {
                return {};
        }

        std::vector<numerical::Matrix<2, 2, T>> predict_f;
        std::vector<numerical::Vector<2, T>> predict_x;
        std::vector<numerical::Matrix<2, 2, T>> predict_p;

        std::vector<numerical::Vector<2, T>> x;
        std::vector<numerical::Matrix<2, 2, T>> p;

        ASSERT(!result[0].info.predict_f);
        ASSERT(!result[0].info.predict_x);
        ASSERT(!result[0].info.predict_p);
        predict_f.push_back(numerical::Matrix<2, 2, T>(numerical::ZERO_MATRIX));
        predict_x.push_back(numerical::Vector<2, T>(0));
        predict_p.push_back(numerical::Matrix<2, 2, T>(numerical::ZERO_MATRIX));

        x.push_back(result[0].info.update_x);
        p.push_back(result[0].info.update_p);

        for (std::size_t i = 1; i < result.size(); ++i)
        {
                const auto& p_f = result[i].info.predict_f;
                const auto& p_x = result[i].info.predict_x;
                const auto& p_p = result[i].info.predict_p;

                if (!(p_f && p_x && p_p))
                {
                        return {};
                }

                predict_f.push_back(*p_f);
                predict_x.push_back(*p_x);
                predict_p.push_back(*p_p);

                x.push_back(result[i].info.update_x);
                p.push_back(result[i].info.update_p);
        }

        std::tie(x, p) = smooth(predict_f, predict_x, predict_p, x, p);

        std::vector<view::Point<T>> res;
        res.reserve(result.size());
        for (std::size_t i = 0; i < result.size(); ++i)
        {
                res.push_back({
                        .time = result[i].time,
                        .x = x[i][0],
                        .x_stddev = std::sqrt(p[i][0, 0]),
                        .v = x[i][1],
                        .v_stddev = std::sqrt(p[i][1, 1]),
                });
        }
        return res;
}

#define INSTANTIATION(T)                                                                         \
        template std::vector<view::Point<T>> view_points(const std::vector<TimeUpdateInfo<T>>&); \
        template std::vector<view::Point<T>> smooth_view_points(const std::vector<TimeUpdateInfo<T>>&);

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
