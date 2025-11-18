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

        const Vector3 a_avg = acc_data_ / T(acc_count_);
        const T a_avg_norm = a_avg.norm();

        if (!acc_suitable(a_avg_norm))
        {
                reset();
                return std::nullopt;
        }

        const Vector3 m_avg = mag_data_ / T(mag_count_);
        const T m_avg_norm = m_avg.norm();

        if (!mag_suitable(m_avg_norm))
        {
                reset();
                return std::nullopt;
        }

        const Vector3 a_normalized = a_avg / a_avg_norm;
        const Vector3 m_normalized = m_avg / m_avg_norm;

        const T cos = dot(a_normalized, m_normalized);

        if (!(cos < MAX_COS_Z_MAG<T>))
        {
                reset();
                return std::nullopt;
        }

        return initial_quaternion(a_normalized, m_normalized);
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
