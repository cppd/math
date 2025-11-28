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

#include "init_marg.h"

#include "constant.h"
#include "init_utility.h"
#include "quaternion.h"

#include <src/com/error.h>
#include <src/filter/attitude/limit.h>

#include <optional>

namespace ns::filter::attitude::kalman
{
namespace
{
template <typename T>
constexpr T MAX_COS_Z_MAG_SQUARED = 1 - square(MIN_SIN_Z_MAG<T>);
}

template <typename T>
InitMarg<T>::InitMarg(const unsigned count)
        : count_(count)
{
        reset();
}

template <typename T>
void InitMarg<T>::reset()
{
        acc_data_ = Vector3(0);
        acc_count_ = 0;

        mag_data_ = Vector3(0);
        mag_count_ = 0;
}

template <typename T>
std::optional<Quaternion<T>> InitMarg<T>::init()
{
        if (acc_count_ < count_ || mag_count_ < count_)
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

        const Vector3 mag = mag_data_ / T(mag_count_);
        const T mag_norm = mag.norm();

        if (!mag_suitable(mag_norm))
        {
                reset();
                return std::nullopt;
        }

        const T cos = dot(acc, mag) / (acc_norm * mag_norm);

        if (!(square(cos) < MAX_COS_Z_MAG_SQUARED<T>))
        {
                reset();
                return std::nullopt;
        }

        return initial_quaternion(acc, mag);
}

template <typename T>
std::optional<Quaternion<T>> InitMarg<T>::update_acc(const Vector3& acc)
{
        ASSERT(acc_count_ < count_ || mag_count_ < count_);

        acc_data_ += acc;
        ++acc_count_;

        return init();
}

template <typename T>
std::optional<Quaternion<T>> InitMarg<T>::update_mag(const Vector3& mag)
{
        ASSERT(acc_count_ < count_ || mag_count_ < count_);

        mag_data_ += mag;
        ++mag_count_;

        return init();
}

template <typename T>
std::optional<Quaternion<T>> InitMarg<T>::update_acc_mag(const Vector3& acc, const Vector3& mag)
{
        ASSERT(acc_count_ < count_ || mag_count_ < count_);

        acc_data_ += acc;
        ++acc_count_;

        mag_data_ += mag;
        ++mag_count_;

        return init();
}

template class InitMarg<float>;
template class InitMarg<double>;
template class InitMarg<long double>;
}
