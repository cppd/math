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
class Data final
{
        Container<numerical::Matrix<N, N, T>> predict_f_;
        Container<numerical::Vector<N, T>> predict_x_;
        Container<numerical::Matrix<N, N, T>> predict_p_;
        Container<numerical::Vector<N, T>> x_;
        Container<numerical::Matrix<N, N, T>> p_;
        Container<T> time_;

public:
        void init(const TimeUpdateDetails<N, T>& details)
        {
                ASSERT(!details.details.predict_f);
                ASSERT(!details.details.predict_x);
                ASSERT(!details.details.predict_p);

                predict_f_.clear();
                predict_x_.clear();
                predict_p_.clear();
                x_.clear();
                p_.clear();
                time_.clear();

                predict_f_.push_back(numerical::Matrix<N, N, T>(numerical::ZERO_MATRIX));
                predict_x_.push_back(numerical::Vector<N, T>(0));
                predict_p_.push_back(numerical::Matrix<N, N, T>(numerical::ZERO_MATRIX));
                x_.push_back(details.details.update_x);
                p_.push_back(details.details.update_p);
                time_.push_back(details.time);
        }

        void push(const TimeUpdateDetails<N, T>& details)
        {
                ASSERT(details.details.predict_f);
                ASSERT(details.details.predict_x);
                ASSERT(details.details.predict_p);

                predict_f_.push_back(*details.details.predict_f);
                predict_x_.push_back(*details.details.predict_x);
                predict_p_.push_back(*details.details.predict_p);
                x_.push_back(details.details.update_x);
                p_.push_back(details.details.update_p);
                time_.push_back(details.time);
        }

        void pop()
        {
                ASSERT(!predict_f_.empty());
                ASSERT(!predict_x_.empty());
                ASSERT(!predict_p_.empty());
                ASSERT(!x_.empty());
                ASSERT(!p_.empty());
                ASSERT(!time_.empty());

                predict_f_.pop_front();
                predict_x_.pop_front();
                predict_p_.pop_front();
                x_.pop_front();
                p_.pop_front();
                time_.pop_front();
        }

        [[nodiscard]] decltype(auto) smooth_all() const
        {
                return core::smooth_all(predict_f_, predict_x_, predict_p_, x_, p_);
        }

        [[nodiscard]] decltype(auto) smooth_first() const
        {
                return core::smooth_first(predict_f_, predict_x_, predict_p_, x_, p_);
        }

        [[nodiscard]] T time(const std::size_t index) const
        {
                ASSERT(index < time_.size());

                return time_[index];
        }

        [[nodiscard]] std::size_t size() const
        {
                return predict_f_.size();
        }
};

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

template <std::size_t N, typename T, std::size_t ORDER, template <typename... Ts> typename Container>
void smooth_all(
        const Data<N, T, Container>& data,
        const filters::FilterPosition<2, T, ORDER>& filter,
        std::vector<view::Point<2, T>>& points)
{
        const auto [x, p] = data.smooth_all();

        ASSERT(x.size() == p.size());
        ASSERT(x.size() == data.size());

        for (std::size_t i = 0; i < x.size(); ++i)
        {
                points.push_back(make_point(data.time(i), x[i], p[i], filter));
        }
}

template <std::size_t N, typename T, std::size_t ORDER, template <typename... Ts> typename Container>
void smooth_first(
        const Data<N, T, Container>& data,
        const filters::FilterPosition<2, T, ORDER>& filter,
        std::vector<view::Point<2, T>>& points)
{
        ASSERT(data.size() > 0);

        const auto [x, p] = data.smooth_first();

        points.push_back(make_point(data.time(0), x, p, filter));
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

        data.init(details[0]);

        for (std::size_t i = 1; i < details.size(); ++i)
        {
                const auto& p_f = details[i].details.predict_f;
                const auto& p_x = details[i].details.predict_x;
                const auto& p_p = details[i].details.predict_p;

                if (!(p_f && p_x && p_p))
                {
                        smooth_all(data, filter, res);
                        data.init(details[i]);
                        continue;
                }

                data.push(details[i]);
        }

        smooth_all(data, filter, res);

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

        data.init(details[0]);

        for (std::size_t i = 1; i < details.size(); ++i)
        {
                const auto& p_f = details[i].details.predict_f;
                const auto& p_x = details[i].details.predict_x;
                const auto& p_p = details[i].details.predict_p;

                if (!(p_f && p_x && p_p))
                {
                        smooth_all(data, filter, res);
                        data.init(details[i]);
                        continue;
                }

                data.push(details[i]);

                if (data.size() < lag + 1)
                {
                        continue;
                }

                smooth_first(data, filter, res);
                data.pop();
        }

        smooth_all(data, filter, res);

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
