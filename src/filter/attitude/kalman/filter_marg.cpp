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

#include "filter_marg.h"

#include "ekf_marg.h"
#include "measurement.h"
#include "ukf_marg.h"

#include <src/com/error.h>
#include <src/filter/attitude/kalman/quaternion.h>
#include <src/filter/attitude/limit.h>
#include <src/numerical/vector.h>

#include <optional>

namespace ns::filter::attitude::kalman
{
template <typename T, template <typename> typename Filter>
void FilterMarg<T, Filter>::init(const Quaternion<T>& q)
{
        ASSERT(init_marg_);
        ASSERT(!filter_);

        init_marg_.reset();
        filter_.emplace(q, init_variance_error_, init_variance_bias_);
}

template <typename T, template <typename> typename Filter>
FilterMarg<T, Filter>::FilterMarg(const unsigned init_count, const T init_variance_error, const T init_variance_bias)
        : init_variance_error_(init_variance_error),
          init_variance_bias_(init_variance_bias),
          init_marg_(init_count)
{
}

template <typename T, template <typename> typename Filter>
void FilterMarg<T, Filter>::update_gyro(
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

template <typename T, template <typename> typename Filter>
void FilterMarg<T, Filter>::update_acc(const numerical::Vector<3, T>& acc, const T acc_variance, const T y_variance)
{
        const T acc_norm = acc.norm();

        if (!acc_suitable(acc_norm))
        {
                return;
        }

        if (init_marg_)
        {
                ASSERT(!filter_);
                const auto init_q = init_marg_->update_acc(acc);
                if (init_q)
                {
                        init(*init_q);
                }
                return;
        }

        ASSERT(filter_);
        filter_->update_z(acc / acc_norm, acc_variance, y_variance);
}

template <typename T, template <typename> typename Filter>
void FilterMarg<T, Filter>::update_mag(const numerical::Vector<3, T>& mag, const T mag_variance, const T z_variance)
{
        const T mag_norm = mag.norm();

        if (!mag_suitable(mag_norm))
        {
                return;
        }

        if (init_marg_)
        {
                ASSERT(!filter_);
                const auto init_q = init_marg_->update_mag(mag);
                if (init_q)
                {
                        init(*init_q);
                }
                return;
        }

        ASSERT(filter_);

        const auto m = mag_measurement(filter_->z_local(), mag / mag_norm, mag_variance);
        if (!m)
        {
                return;
        }

        filter_->update_y(m->y, m->variance, z_variance);
}

template <typename T, template <typename> typename Filter>
void FilterMarg<T, Filter>::update_acc_mag(
        const numerical::Vector<3, T>& acc,
        const numerical::Vector<3, T>& mag,
        const T acc_variance,
        const T mag_variance)
{
        const T acc_norm = acc.norm();

        if (!acc_suitable(acc_norm))
        {
                return;
        }

        const T mag_norm = mag.norm();

        if (!mag_suitable(mag_norm))
        {
                return;
        }

        if (init_marg_)
        {
                ASSERT(!filter_);
                const auto init_q = init_marg_->update_acc_mag(acc, mag);
                if (init_q)
                {
                        init(*init_q);
                }
                return;
        }

        ASSERT(filter_);

        const auto m = mag_measurement(filter_->z_local(), mag / mag_norm, mag_variance);
        if (!m)
        {
                return;
        }

        filter_->update_z_y(acc / acc_norm, m->y, acc_variance, m->variance);
}

#define TEMPLATE(T)                                                                       \
        template class FilterMarg<T, EkfMarg>;                                            \
        template void FilterMarg<T, UkfMarg>::init(const Quaternion<T>&);                 \
        template FilterMarg<T, UkfMarg>::FilterMarg(unsigned, T, T);                      \
        template void FilterMarg<T, UkfMarg>::update_gyro(                                \
                const numerical::Vector<3, T>&, const numerical::Vector<3, T>&, T, T, T); \
        template void FilterMarg<T, UkfMarg>::update_acc_mag(                             \
                const numerical::Vector<3, T>&, const numerical::Vector<3, T>&, T, T);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
