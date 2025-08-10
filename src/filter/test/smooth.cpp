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

#include "smooth.h"

#include "time_update_details.h"

#include "view/point.h"

#include <src/com/error.h>
#include <src/filter/core/smooth.h>
#include <src/filter/filters/filter.h>
#include <src/filter/settings/instantiation.h>
#include <src/numerical/matrix_object.h>
#include <src/numerical/vector_object.h>

#include <cstddef>
#include <deque>
#include <vector>

namespace ns::filter::test
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
void init(const TimeUpdateDetails<N, T>& details, Data<N, T, Container>& data)
{
        ASSERT(!details.details.predict_f);
        ASSERT(!details.details.predict_x);
        ASSERT(!details.details.predict_p);

        data.predict_f.clear();
        data.predict_x.clear();
        data.predict_p.clear();
        data.x.clear();
        data.p.clear();
        data.time.clear();

        data.predict_f.push_back(numerical::Matrix<N, N, T>(numerical::ZERO_MATRIX));
        data.predict_x.push_back(numerical::Vector<N, T>(0));
        data.predict_p.push_back(numerical::Matrix<N, N, T>(numerical::ZERO_MATRIX));
        data.x.push_back(details.details.update_x);
        data.p.push_back(details.details.update_p);
        data.time.push_back(details.time);
}

template <std::size_t N, typename T, template <typename... Ts> typename Container>
void push(const TimeUpdateDetails<N, T>& details, Data<N, T, Container>& data)
{
        ASSERT(details.details.predict_f);
        ASSERT(details.details.predict_x);
        ASSERT(details.details.predict_p);

        data.predict_f.push_back(*details.details.predict_f);
        data.predict_x.push_back(*details.details.predict_x);
        data.predict_p.push_back(*details.details.predict_p);
        data.x.push_back(details.details.update_x);
        data.p.push_back(details.details.update_p);
        data.time.push_back(details.time);
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

template <std::size_t N, typename T, std::size_t ORDER>
view::Point<2, T> make_point(
        const T time,
        const numerical::Vector<N, T>& x,
        const numerical::Matrix<N, N, T>& p,
        const filters::FilterPosition<2, T, ORDER>& filter)
{
        T speed = 0;
        T speed_p = 0;

        if constexpr (ORDER > 0)
        {
                speed = filter.x_to_speed(x);
                speed_p = filter.xp_to_speed_p(x, p);
        }

        return {
                .time = time,
                .position = filter.x_to_position(x),
                .position_p = filter.p_to_position_p(p),
                .speed = speed,
                .speed_p = speed_p,
        };
}

template <std::size_t N, typename T, std::size_t ORDER>
std::vector<view::Point<2, T>> copy_x_p(
        const std::vector<TimeUpdateDetails<N, T>>& details,
        const filters::FilterPosition<2, T, ORDER>& filter)
{
        std::vector<view::Point<2, T>> res;
        res.reserve(details.size());

        for (std::size_t i = 0; i < details.size(); ++i)
        {
                res.push_back(
                        make_point(details[i].time, details[i].details.update_x, details[i].details.update_p, filter));
        }

        return res;
}

template <std::size_t N, typename T, std::size_t ORDER>
void write_smooth(
        const filters::FilterPosition<2, T, ORDER>& filter,
        const Data<N, T, std::vector>& data,
        std::vector<view::Point<2, T>>& points)
{
        const auto [x, p] = core::smooth_all(data.predict_f, data.predict_x, data.predict_p, data.x, data.p);

        ASSERT(x.size() == p.size());
        ASSERT(x.size() == data.time.size());

        for (std::size_t i = 0; i < x.size(); ++i)
        {
                points.push_back(make_point(data.time[i], x[i], p[i], filter));
        }
}

template <std::size_t N, typename T, std::size_t ORDER>
void write_smooth(
        const filters::FilterPosition<2, T, ORDER>& filter,
        const Data<N, T, std::deque>& data,
        std::vector<view::Point<2, T>>& points)
{
        const auto [x, p] = core::smooth_all(data.predict_f, data.predict_x, data.predict_p, data.x, data.p);

        ASSERT(data.x.size() == x.size());
        ASSERT(data.x.size() == p.size());

        for (std::size_t i = 0; i < x.size(); ++i)
        {
                points.push_back(make_point(data.time[i], x[i], p[i], filter));
        }
}
}

template <std::size_t N, typename T, std::size_t ORDER>
std::vector<view::Point<2, T>> smooth_all(
        const filters::FilterPosition<2, T, ORDER>& filter,
        const std::vector<TimeUpdateDetails<N, T>>& details)
{
        static_assert(N >= 2);
        static_assert((N % 2) == 0);

        if (details.empty())
        {
                return {};
        }

        std::vector<view::Point<2, T>> res;
        res.reserve(details.size());

        Data<N, T, std::vector> data;

        init(details[0], data);

        for (std::size_t i = 1; i < details.size(); ++i)
        {
                const auto& p_f = details[i].details.predict_f;
                const auto& p_x = details[i].details.predict_x;
                const auto& p_p = details[i].details.predict_p;

                if (!(p_f && p_x && p_p))
                {
                        write_smooth(filter, data, res);
                        init(details[i], data);
                        continue;
                }

                push(details[i], data);
        }

        write_smooth(filter, data, res);

        return res;
}

template <std::size_t N, typename T, std::size_t ORDER>
std::vector<view::Point<2, T>> smooth_lag(
        const filters::FilterPosition<2, T, ORDER>& filter,
        const std::vector<TimeUpdateDetails<N, T>>& details,
        const unsigned lag)
{
        static_assert(N >= 2);
        static_assert((N % 2) == 0);

        if (details.empty())
        {
                return {};
        }

        if (lag == 0)
        {
                return copy_x_p(details, filter);
        }

        std::vector<view::Point<2, T>> res;

        Data<N, T, std::deque> data;

        init(details[0], data);

        for (std::size_t i = 1; i < details.size(); ++i)
        {
                const auto& p_f = details[i].details.predict_f;
                const auto& p_x = details[i].details.predict_x;
                const auto& p_p = details[i].details.predict_p;

                if (!(p_f && p_x && p_p))
                {
                        write_smooth(filter, data, res);
                        init(details[i], data);
                        continue;
                }

                push(details[i], data);

                if (data.x.size() < lag + 1)
                {
                        continue;
                }

                const auto [x, p] = core::smooth_first(data.predict_f, data.predict_x, data.predict_p, data.x, data.p);

                res.push_back(make_point(data.time.front(), x, p, filter));

                pop(data);
        }

        write_smooth(filter, data, res);

        ASSERT(res.size() == details.size());
        return res;
}

#define TEMPLATE_N(T, ORDER, N)                                                                                \
        template std::vector<view::Point<2, T>> smooth_all(                                                    \
                const filters::FilterPosition<2, T, (ORDER)>&, const std::vector<TimeUpdateDetails<(N), T>>&); \
        template std::vector<view::Point<2, T>> smooth_lag(                                                    \
                const filters::FilterPosition<2, T, (ORDER)>&, const std::vector<TimeUpdateDetails<(N), T>>&,  \
                unsigned);

#define TEMPLATE(T)         \
        TEMPLATE_N(T, 0, 2) \
        TEMPLATE_N(T, 1, 4) \
        TEMPLATE_N(T, 2, 6)

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)

}
