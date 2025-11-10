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
#include <src/filter/attitude/kalman/init_imu.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <optional>

namespace ns::filter::attitude::kalman
{
template <typename T, template <typename> typename Filter>
class FilterImu final
{
        const T init_variance_;
        std::optional<InitImu<T>> init_imu_;
        std::optional<Filter<T>> filter_;

public:
        FilterImu(const unsigned init_count, const T init_variance)
                : init_variance_(init_variance),
                  init_imu_(init_count)
        {
        }

        void update_gyro(
                const numerical::Vector<3, T>& w0,
                const numerical::Vector<3, T>& w1,
                const T variance,
                const T dt)
        {
                if (init_imu_)
                {
                        ASSERT(!filter_);
                        return;
                }

                ASSERT(filter_);
                filter_->update_gyro(w0, w1, variance, dt);
        }

        bool update_acc(const numerical::Vector<3, T>& a, const T variance, const T variance_direction)
        {
                if (init_imu_)
                {
                        ASSERT(!filter_);
                        const auto init_q = init_imu_->update(a);
                        if (init_q)
                        {
                                init_imu_.reset();
                                filter_.emplace(*init_q, init_variance_);
                        }
                        return false;
                }

                ASSERT(filter_);
                return filter_->update_acc(a, variance, variance_direction);
        }

        [[nodiscard]] numerical::Quaternion<T> attitude() const
        {
                ASSERT(filter_);
                return filter_->attitude();
        }
};
}
