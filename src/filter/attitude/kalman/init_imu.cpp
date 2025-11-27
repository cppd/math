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

#include "init_imu.h"

#include "init_utility.h"
#include "quaternion.h"

#include <src/com/error.h>
#include <src/filter/attitude/limit.h>

#include <optional>

namespace ns::filter::attitude::kalman
{
template <typename T>
InitImu<T>::InitImu(const unsigned count)
        : count_(count)
{
        reset();
}

template <typename T>
void InitImu<T>::reset()
{
        acc_data_ = Vector3(0);
        acc_count_ = 0;
}

template <typename T>
std::optional<Quaternion<T>> InitImu<T>::init()
{
        if (acc_count_ < count_)
        {
                return std::nullopt;
        }

        const Vector3 acc = acc_data_ / T(acc_count_);
        const T acc_norm = acc.norm();

        if (!acc_suitable(acc_norm))
        {
                reset();
                return std::nullopt;
        }

        return initial_quaternion(acc / acc_norm);
}

template <typename T>
std::optional<Quaternion<T>> InitImu<T>::update(const Vector3& acc)
{
        ASSERT(acc_count_ < count_);

        acc_data_ += acc;
        ++acc_count_;

        return init();
}

template class InitImu<float>;
template class InitImu<double>;
template class InitImu<long double>;
}
