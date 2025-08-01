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
        data.time.clear();

        data.predict_f.push_back(numerical::Matrix<N, N, T>(numerical::ZERO_MATRIX));
        data.predict_x.push_back(numerical::Vector<N, T>(0));
        data.predict_p.push_back(numerical::Matrix<N, N, T>(numerical::ZERO_MATRIX));
        data.x.push_back(info.info.update_x);
        data.p.push_back(info.info.update_p);
        data.time.push_back(info.time);
}

template <std::size_t N, typename T, template <typename... Ts> typename Container>
void push(const TimeUpdateInfo<T>& info, Data<N, T, Container>& data)
{
        ASSERT(info.info.predict_f);
        ASSERT(info.info.predict_x);
        ASSERT(info.info.predict_p);

        data.predict_f.push_back(*info.info.predict_f);
        data.predict_x.push_back(*info.info.predict_x);
        data.predict_p.push_back(*info.info.predict_p);
        data.x.push_back(info.info.update_x);
        data.p.push_back(info.info.update_p);
        data.time.push_back(info.time);
}

template <std::size_t N, typename T, template <typename... Ts> typename Container>
void pop(Data<N, T, Container>& data)
{
        ASSERT(!data.predict_f.empty());
        ASSERT(!data.predict_x.empty());
        ASSERT(!data.predict_p.empty());
        ASSERT(!data.x.empty());
        ASSERT(!data.p.empty());
        ASSERT(!data.time.empty());

        data.predict_f.pop_front();
        data.predict_x.pop_front();
        data.predict_p.pop_front();
        data.x.pop_front();
        data.p.pop_front();
        data.time.pop_front();
}

template <typename T>
std::vector<T> to_vector(const std::deque<T>& d)
{
        return {d.cbegin(), d.cend()};
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

template <typename T>
std::vector<view::Point<T>> copy_x_p(const std::vector<TimeUpdateInfo<T>>& info)
{
        std::vector<view::Point<T>> res;
        res.reserve(info.size());

        for (std::size_t i = 0; i < info.size(); ++i)
        {
                res.push_back(make_point(info[i].time, info[i].info.update_x, info[i].info.update_p));
        }

        return res;
}

template <typename T>
bool check_predict(const std::vector<TimeUpdateInfo<T>>& info)
{
        for (std::size_t i = 1; i < info.size(); ++i)
        {
                const auto& p_f = info[i].info.predict_f;
                const auto& p_x = info[i].info.predict_x;
                const auto& p_p = info[i].info.predict_p;
                if (!(p_f && p_x && p_p))
                {
                        return false;
                }
        }
        return true;
}

template <typename T>
void write_smooth(const Data<2, T, std::vector>& data, std::vector<view::Point<T>>& res)
{
        const auto [x, p] = core::smooth(data.predict_f, data.predict_x, data.predict_p, data.x, data.p);

        ASSERT(x.size() == p.size());
        ASSERT(x.size() == data.time.size());

        for (std::size_t i = 0; i < x.size(); ++i)
        {
                res.push_back(make_point(data.time[i], x[i], p[i]));
        }
}
}

template <typename T>
std::vector<view::Point<T>> view_points(const std::vector<TimeUpdateInfo<T>>& info)
{
        std::vector<view::Point<T>> res;
        res.reserve(info.size());
        for (const TimeUpdateInfo<T>& r : info)
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
std::vector<view::Point<T>> smooth_view_points_all(const std::vector<TimeUpdateInfo<T>>& info)
{
        if (info.empty())
        {
                return {};
        }

        std::vector<view::Point<T>> res;

        Data<2, T, std::vector> data;

        init(info[0], data);

        for (std::size_t i = 1; i < info.size(); ++i)
        {
                const auto& p_f = info[i].info.predict_f;
                const auto& p_x = info[i].info.predict_x;
                const auto& p_p = info[i].info.predict_p;

                if (!(p_f && p_x && p_p))
                {
                        write_smooth(data, res);
                        init(info[i], data);
                        continue;
                }

                push(info[i], data);
        }

        write_smooth(data, res);

        return res;
}

template <typename T>
std::vector<view::Point<T>> smooth_view_points_lag(const std::vector<TimeUpdateInfo<T>>& info, const unsigned lag)
{
        if (info.empty())
        {
                return {};
        }

        if (!check_predict(info))
        {
                return {};
        }

        if (lag == 0)
        {
                return copy_x_p(info);
        }

        std::vector<view::Point<T>> res;
        res.reserve(info.size());

        Data<2, T, std::deque> data;

        init(info[0], data);

        for (std::size_t i = 1; i < info.size(); ++i)
        {
                push(info[i], data);

                if (data.x.size() < lag + 1)
                {
                        continue;
                }

                const auto [xs, ps] = smooth(data.predict_f, data.predict_x, data.predict_p, data.x, data.p);

                ASSERT(i >= lag);
                res.push_back(make_point(info[i - lag].time, xs, ps));

                pop(data);
        }

        const auto [x, p] = smooth(
                to_vector(data.predict_f), to_vector(data.predict_x), to_vector(data.predict_p), to_vector(data.x),
                to_vector(data.p));

        ASSERT(info.size() >= x.size());
        for (std::size_t i = info.size() - x.size(), j = 0; i < info.size(); ++i, ++j)
        {
                res.push_back(make_point(info[i].time, x[j], p[j]));
        }

        ASSERT(res.size() == info.size());

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
