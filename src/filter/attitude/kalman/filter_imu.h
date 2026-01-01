/*
Copyright (C) 2017-2026 Topological Manifold

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
        FilterImu(unsigned init_count, T init_variance);

        void update_gyro(const numerical::Vector<3, T>& w0, const numerical::Vector<3, T>& w1, T variance, T dt);

        void update_acc(const numerical::Vector<3, T>& acc, T acc_variance, T y_variance);

        [[nodiscard]] std::optional<numerical::Quaternion<T>> attitude() const
        {
                if (filter_)
                {
                        return numerical::Quaternion<T>(filter_->attitude());
                }
                return std::nullopt;
        }
};
}
