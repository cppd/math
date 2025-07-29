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
template <std::size_t N, typename T, template <typename... Ts> typename Container>
struct Data
{
        Container<numerical::Matrix<N, N, T>> predict_f;
        Container<numerical::Vector<N, T>> predict_x;
        Container<numerical::Matrix<N, N, T>> predict_p;
        Container<numerical::Vector<N, T>> x;
        Container<numerical::Matrix<N, N, T>> p;
        Container<T> time;
};

template <std::size_t N, typename T, template <typename... Ts> typename Container>
void init(const TimeUpdateInfo<T>& info, Data<N, T, Container>& data)
{
        ASSERT(!info.info.predict_f);
        ASSERT(!info.info.predict_x);
        ASSERT(!info.info.predict_p);

        data.predict_f.clear();
        data.predict_x.clear();
        data.predict_p.clear();
        data.x.clear();
        data.p.clear();

        data.predict_f.push_back(numerical::Matrix<N, N, T>(numerical::ZERO_MATRIX));
        data.predict_x.push_back(numerical::Vector<N, T>(0));
        data.predict_p.push_back(numerical::Matrix<N, N, T>(numerical::ZERO_MATRIX));
        data.x.push_back(info.info.update_x);
        data.p.push_back(info.info.update_p);
}

template <std::size_t N, typename T, template <typename... Ts> typename Container>
void add(const TimeUpdateInfo<T>& info, Data<N, T, Container>& data)
{
        ASSERT(info.info.predict_f);
        ASSERT(info.info.predict_x);
        ASSERT(info.info.predict_p);

        data.predict_f.push_back(*info.info.predict_f);
        data.predict_x.push_back(*info.info.predict_x);
        data.predict_p.push_back(*info.info.predict_p);
        data.x.push_back(info.info.update_x);
        data.p.push_back(info.info.update_p);
}

template <std::size_t N, typename T, template <typename... Ts> typename Container>
void correct_size(const unsigned max_size, Data<N, T, Container>& data)
{
        ASSERT(data.x.size() == data.p.size());
        ASSERT(data.x.size() == data.predict_f.size());
        ASSERT(data.x.size() == data.predict_x.size());
        ASSERT(data.x.size() == data.predict_p.size());

        while (data.x.size() > max_size)
        {
                data.predict_f.pop_front();
                data.predict_x.pop_front();
                data.predict_p.pop_front();
                data.x.pop_front();
                data.p.pop_front();
        }
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

        Data<2, T, std::vector> data;

        init(result[0], data);

        for (std::size_t i = 1; i < result.size(); ++i)
        {
                const auto& p_f = result[i].info.predict_f;
                const auto& p_x = result[i].info.predict_x;
                const auto& p_p = result[i].info.predict_p;

                if (!(p_f && p_x && p_p))
                {
                        return {};
                }

                add(result[i], data);
        }

        const auto [x, p] = smooth(data.predict_f, data.predict_x, data.predict_p, data.x, data.p);

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

        Data<2, T, std::deque> data;

        init(result[0], data);

        for (std::size_t i = 1; i < result.size(); ++i)
        {
                const auto& p_f = result[i].info.predict_f;
                const auto& p_x = result[i].info.predict_x;
                const auto& p_p = result[i].info.predict_p;

                if (!(p_f && p_x && p_p))
                {
                        return {};
                }

                correct_size(count - 1, data);
                add(result[i], data);

                if (data.x.size() < count)
                {
                        continue;
                }

                const auto [xs, ps] = smooth(data.predict_f, data.predict_x, data.predict_p, data.x, data.p);

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
