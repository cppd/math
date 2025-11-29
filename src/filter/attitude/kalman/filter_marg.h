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

#include <src/filter/attitude/kalman/init_marg.h>
#include <src/filter/attitude/kalman/quaternion.h>
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

        void init(const Quaternion<T>& q);

public:
        FilterMarg(unsigned init_count, T init_variance_error, T init_variance_bias);

        void update_gyro(
                const numerical::Vector<3, T>& w0,
                const numerical::Vector<3, T>& w1,
                T variance_r,
                T variance_w,
                T dt);

        void update_acc(const numerical::Vector<3, T>& acc, T acc_variance, T y_variance);

        void update_mag(const numerical::Vector<3, T>& mag, T mag_variance, T z_variance);

        void update_acc_mag(
                const numerical::Vector<3, T>& acc,
                const numerical::Vector<3, T>& mag,
                T acc_variance,
                T mag_variance);

        [[nodiscard]] std::optional<numerical::Quaternion<T>> attitude() const
        {
                if (filter_)
                {
                        return numerical::Quaternion<T>(filter_->attitude());
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
