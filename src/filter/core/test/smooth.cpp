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

#include "smooth.h"

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
class Data final
{
        Container<numerical::Matrix<N, N, T>> predict_f_;
        Container<numerical::Vector<N, T>> predict_x_;
        Container<numerical::Matrix<N, N, T>> predict_p_;
        Container<numerical::Vector<N, T>> x_;
        Container<numerical::Matrix<N, N, T>> p_;
        Container<T> time_;

public:
        void init(const TimeUpdateInfo<T>& info)
        {
                ASSERT(!info.info.predict_f);
                ASSERT(!info.info.predict_x);
                ASSERT(!info.info.predict_p);

                predict_f_.clear();
                predict_x_.clear();
                predict_p_.clear();
                x_.clear();
                p_.clear();
                time_.clear();

                predict_f_.push_back(numerical::Matrix<N, N, T>(numerical::ZERO_MATRIX));
                predict_x_.push_back(numerical::Vector<N, T>(0));
                predict_p_.push_back(numerical::Matrix<N, N, T>(numerical::ZERO_MATRIX));
                x_.push_back(info.info.update_x);
                p_.push_back(info.info.update_p);
                time_.push_back(info.time);
        }

        void push(const TimeUpdateInfo<T>& info)
        {
                ASSERT(info.info.predict_f);
                ASSERT(info.info.predict_x);
                ASSERT(info.info.predict_p);

                predict_f_.push_back(*info.info.predict_f);
                predict_x_.push_back(*info.info.predict_x);
                predict_p_.push_back(*info.info.predict_p);
                x_.push_back(info.info.update_x);
                p_.push_back(info.info.update_p);
                time_.push_back(info.time);
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

        [[nodiscard]] std::size_t size() const
        {
                return predict_f_.size();
        }

        [[nodiscard]] const Container<numerical::Matrix<N, N, T>>& predict_f() const
        {
                return predict_f_;
        }

        [[nodiscard]] const Container<numerical::Vector<N, T>>& predict_x() const
        {
                return predict_x_;
        }

        [[nodiscard]] const Container<numerical::Matrix<N, N, T>>& predict_p() const
        {
                return predict_p_;
        }

        [[nodiscard]] const Container<numerical::Vector<N, T>>& x() const
        {
                return x_;
        }

        [[nodiscard]] const Container<numerical::Matrix<N, N, T>>& p() const
        {
                return p_;
        }

        [[nodiscard]] const Container<T>& time() const
        {
                return time_;
        }
};

template <typename T>
view::Point<T> make_point(const T time, const numerical::Vector<2, T>& x, const numerical::Matrix<2, 2, T>& p)
{
        return {
                .time = time,
                .position = x[0],
                .position_stddev = std::sqrt(p[0, 0]),
                .speed = x[1],
                .speed_stddev = std::sqrt(p[1, 1]),
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
void write_smooth(const Data<2, T, std::vector>& data, std::vector<view::Point<T>>& res)
{
        const auto [x, p] = core::smooth_all(data.predict_f(), data.predict_x(), data.predict_p(), data.x(), data.p());

        ASSERT(x.size() == p.size());
        ASSERT(x.size() == data.size());

        for (std::size_t i = 0; i < x.size(); ++i)
        {
                res.push_back(make_point(data.time()[i], x[i], p[i]));
        }
}

template <typename T>
void write_smooth(const Data<2, T, std::deque>& data, std::vector<view::Point<T>>& res)
{
        const auto [x, p] = core::smooth_all(data.predict_f(), data.predict_x(), data.predict_p(), data.x(), data.p());

        ASSERT(x.size() == p.size());
        ASSERT(x.size() == data.size());

        for (std::size_t i = 0; i < x.size(); ++i)
        {
                res.push_back(make_point(data.time()[i], x[i], p[i]));
        }
}
}

template <typename T>
std::vector<view::Point<T>> smooth_all(const std::vector<TimeUpdateInfo<T>>& info)
{
        if (info.empty())
        {
                return {};
        }

        std::vector<view::Point<T>> res;
        res.reserve(info.size());

        Data<2, T, std::vector> data;

        data.init(info[0]);

        for (std::size_t i = 1; i < info.size(); ++i)
        {
                const auto& p_f = info[i].info.predict_f;
                const auto& p_x = info[i].info.predict_x;
                const auto& p_p = info[i].info.predict_p;

                if (!(p_f && p_x && p_p))
                {
                        write_smooth(data, res);
                        data.init(info[i]);
                        continue;
                }

                data.push(info[i]);
        }

        write_smooth(data, res);

        return res;
}

template <typename T>
std::vector<view::Point<T>> smooth_lag(const std::vector<TimeUpdateInfo<T>>& info, const unsigned lag)
{
        if (info.empty())
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

        data.init(info[0]);

        for (std::size_t i = 1; i < info.size(); ++i)
        {
                const auto& p_f = info[i].info.predict_f;
                const auto& p_x = info[i].info.predict_x;
                const auto& p_p = info[i].info.predict_p;

                if (!(p_f && p_x && p_p))
                {
                        write_smooth(data, res);
                        data.init(info[i]);
                        continue;
                }

                data.push(info[i]);

                if (data.size() < lag + 1)
                {
                        continue;
                }

                const auto [xs, ps] =
                        core::smooth_first(data.predict_f(), data.predict_x(), data.predict_p(), data.x(), data.p());

                res.push_back(make_point(data.time().front(), xs, ps));

                data.pop();
        }

        write_smooth(data, res);

        ASSERT(res.size() == info.size());
        return res;
}

#define INSTANTIATION(T)                                                                        \
        template std::vector<view::Point<T>> smooth_all(const std::vector<TimeUpdateInfo<T>>&); \
        template std::vector<view::Point<T>> smooth_lag(const std::vector<TimeUpdateInfo<T>>&, unsigned);

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
