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

#include "filter_imu.h"

#include "ekf_imu.h"
#include "ukf_imu.h"

#include <src/com/error.h>
#include <src/filter/attitude/limit.h>
#include <src/numerical/vector.h>

#include <optional>

namespace ns::filter::attitude::kalman
{
template <typename T, template <typename> typename Filter>
FilterImu<T, Filter>::FilterImu(const unsigned init_count, const T init_variance)
        : init_variance_(init_variance),
          init_imu_(init_count)
{
}

template <typename T, template <typename> typename Filter>
void FilterImu<T, Filter>::update_gyro(
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

template <typename T, template <typename> typename Filter>
void FilterImu<T, Filter>::update_acc(const numerical::Vector<3, T>& acc, const T acc_variance, const T y_variance)
{
        const T acc_norm = acc.norm();

        if (!acc_suitable(acc_norm))
        {
                return;
        }

        if (init_imu_)
        {
                ASSERT(!filter_);
                const auto init_q = init_imu_->update(acc);
                if (init_q)
                {
                        init_imu_.reset();
                        filter_.emplace(*init_q, init_variance_);
                }
                return;
        }

        ASSERT(filter_);
        filter_->update_z(acc / acc_norm, acc_variance, y_variance);
}

#define TEMPLATE(T)                          \
        template class FilterImu<T, EkfImu>; \
        template class FilterImu<T, UkfImu>;

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
