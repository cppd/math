/*
Copyright (Container) 2017-2025 Topological Manifold

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
#include <deque>
#include <vector>

namespace ns::filter::core::test
{
namespace
{
template <typename T, template <typename... Ts> typename Container>
void init(
        const std::vector<TimeUpdateInfo<T>>& result,
        Container<numerical::Matrix<2, 2, T>>& predict_f,
        Container<numerical::Vector<2, T>>& predict_x,
        Container<numerical::Matrix<2, 2, T>>& predict_p,
        Container<numerical::Vector<2, T>>& x,
        Container<numerical::Matrix<2, 2, T>>& p)
{
        ASSERT(!result.empty());

        ASSERT(predict_f.empty());
        ASSERT(predict_x.empty());
        ASSERT(predict_p.empty());
        ASSERT(x.empty());
        ASSERT(p.empty());

        ASSERT(!result[0].info.predict_f);
        ASSERT(!result[0].info.predict_x);
        ASSERT(!result[0].info.predict_p);
        predict_f.push_back(numerical::Matrix<2, 2, T>(numerical::ZERO_MATRIX));
        predict_x.push_back(numerical::Vector<2, T>(0));
        predict_p.push_back(numerical::Matrix<2, 2, T>(numerical::ZERO_MATRIX));

        x.push_back(result[0].info.update_x);
        p.push_back(result[0].info.update_p);
}

template <typename T>
view::Point<T> make_point(const T time, const numerical::Vector<2, T>& x, const numerical::Matrix<2, 2, T>& p)
{
        return {
                .time = time,
                .x = x[0],
                .x_stddev = std::sqrt(p[0, 0]),
                .v = x[1],
                .v_stddev = std::sqrt(p[1, 1]),
        };
}
}

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
std::vector<view::Point<T>> smooth_view_points_all(const std::vector<TimeUpdateInfo<T>>& result)
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

        init(result, predict_f, predict_x, predict_p, x, p);

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
                res.push_back(make_point(result[i].time, x[i], p[i]));
        }
        return res;
}

template <typename T>
std::vector<view::Point<T>> smooth_view_points_lag(const std::vector<TimeUpdateInfo<T>>& result, const unsigned lag)
{
        const unsigned count = lag + 1;

        if (result.size() < count)
        {
                return {};
        }

        std::vector<view::Point<T>> res;
        res.reserve(result.size());

        std::deque<numerical::Matrix<2, 2, T>> predict_f;
        std::deque<numerical::Vector<2, T>> predict_x;
        std::deque<numerical::Matrix<2, 2, T>> predict_p;

        std::deque<numerical::Vector<2, T>> x;
        std::deque<numerical::Matrix<2, 2, T>> p;

        init(result, predict_f, predict_x, predict_p, x, p);

        for (std::size_t i = 1; i < result.size(); ++i)
        {
                const auto& p_f = result[i].info.predict_f;
                const auto& p_x = result[i].info.predict_x;
                const auto& p_p = result[i].info.predict_p;

                if (!(p_f && p_x && p_p))
                {
                        return {};
                }

                while (x.size() >= count)
                {
                        predict_f.pop_front();
                        predict_x.pop_front();
                        predict_p.pop_front();

                        x.pop_front();
                        p.pop_front();
                }

                predict_f.push_back(*p_f);
                predict_x.push_back(*p_x);
                predict_p.push_back(*p_p);

                x.push_back(result[i].info.update_x);
                p.push_back(result[i].info.update_p);

                if (x.size() < count)
                {
                        continue;
                }

                const auto [xs, ps] = smooth(predict_f, predict_x, predict_p, x, p);

                ASSERT(i >= lag);
                res.push_back(make_point(result[i - lag].time, xs, ps));
        }

        return res;
}

#define INSTANTIATION(T)                                                                                    \
        template std::vector<view::Point<T>> view_points(const std::vector<TimeUpdateInfo<T>>&);            \
        template std::vector<view::Point<T>> smooth_view_points_all(const std::vector<TimeUpdateInfo<T>>&); \
        template std::vector<view::Point<T>> smooth_view_points_lag(const std::vector<TimeUpdateInfo<T>>&, unsigned);

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
