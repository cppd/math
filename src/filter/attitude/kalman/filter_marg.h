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

#pragma once

#include <src/com/error.h>
#include <src/filter/attitude/kalman/init_marg.h>
#include <src/filter/attitude/kalman/quaternion.h>
#include <src/filter/attitude/limit.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <optional>

namespace ns::filter::attitude::kalman
{
template <typename T, template <typename> typename Filter>
class FilterMarg final
{
        const T init_variance_error_;
        const T init_variance_bias_;
        std::optional<InitMarg<T>> init_marg_;
        std::optional<Filter<T>> filter_;

        void init(const Quaternion<T>& q)
        {
                ASSERT(init_marg_);
                ASSERT(!filter_);

                init_marg_.reset();
                filter_.emplace(q, init_variance_error_, init_variance_bias_);
        }

public:
        FilterMarg(const unsigned init_count, const T init_variance_error, const T init_variance_bias)
                : init_variance_error_(init_variance_error),
                  init_variance_bias_(init_variance_bias),
                  init_marg_(init_count)
        {
        }

        void update_gyro(
                const numerical::Vector<3, T>& w0,
                const numerical::Vector<3, T>& w1,
                const T variance_r,
                const T variance_w,
                const T dt)
        {
                if (init_marg_)
                {
                        ASSERT(!filter_);
                        return;
                }

                ASSERT(filter_);
                filter_->update_gyro(w0, w1, variance_r, variance_w, dt);
        }

        void update_acc(const numerical::Vector<3, T>& a, const T variance, const T variance_direction)
        {
                if (!acc_suitable(a))
                {
                        return;
                }

                if (init_marg_)
                {
                        ASSERT(!filter_);
                        const auto init_q = init_marg_->update_acc(a);
                        if (init_q)
                        {
                                init(*init_q);
                        }
                        return;
                }

                ASSERT(filter_);
                filter_->update_acc(a, variance, variance_direction);
        }

        void update_mag(const numerical::Vector<3, T>& m, const T variance, const T variance_direction)
        {
                if (!mag_suitable(m))
                {
                        return;
                }

                if (init_marg_)
                {
                        ASSERT(!filter_);
                        const auto init_q = init_marg_->update_mag(m);
                        if (init_q)
                        {
                                init(*init_q);
                        }
                        return;
                }

                ASSERT(filter_);
                filter_->update_mag(m, variance, variance_direction);
        }

        void update_acc_mag(
                const numerical::Vector<3, T>& a,
                const numerical::Vector<3, T>& m,
                const T a_variance,
                const T m_variance)
        {
                if (!acc_suitable(a))
                {
                        return;
                }

                if (!mag_suitable(m))
                {
                        return;
                }

                if (init_marg_)
                {
                        ASSERT(!filter_);
                        const auto init_q = init_marg_->update_acc_mag(a, m);
                        if (init_q)
                        {
                                init(*init_q);
                        }
                        return;
                }

                ASSERT(filter_);
                filter_->update_acc_mag(a, m, a_variance, m_variance);
        }

        [[nodiscard]] std::optional<numerical::Quaternion<T>> attitude() const
        {
                if (filter_)
                {
                        return filter_->attitude();
                }
                return std::nullopt;
        }

        [[nodiscard]] std::optional<numerical::Vector<3, T>> bias() const
        {
                if (filter_)
                {
                        return filter_->bias();
                }
                return std::nullopt;
        }
};
}
