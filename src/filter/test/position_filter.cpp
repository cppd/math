/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "position_filter.h"

#include <src/com/error.h>
#include <src/com/exponent.h>

namespace ns::filter::test
{
namespace
{
template <typename T>
constexpr Matrix<2, 6, T> H{
        {1, 0, 0, 0, 0, 0},
        {0, 0, 0, 1, 0, 0}
};

template <typename T>
constexpr Matrix<6, 2, T> H_T = H<T>.transposed();

template <typename T>
Matrix<6, 6, T> f(const T dt)
{
        const T dt_2 = square(dt) / 2;
        return {
                {1, dt, dt_2, 0,  0,    0},
                {0,  1,   dt, 0,  0,    0},
                {0,  0,    1, 0,  0,    0},
                {0,  0,    0, 1, dt, dt_2},
                {0,  0,    0, 0,  1,   dt},
                {0,  0,    0, 0,  0,    1}
        };
}

template <typename T>
Matrix<6, 6, T> q(const T dt, const T process_variance)
{
        const T dt_2 = square(dt) / 2;
        const Matrix<6, 2, T> noise_transition{
                {dt_2,    0},
                {  dt,    0},
                {   1,    0},
                {   0, dt_2},
                {   0,   dt},
                {   0,    1},
        };

        const T p = process_variance;
        const Matrix<2, 2, T> process_covariance{
                {p, 0},
                {0, p}
        };

        return noise_transition * process_covariance * noise_transition.transposed();
}

template <typename T>
Matrix<2, 2, T> r(const T measurement_variance)
{
        const T mv = measurement_variance;
        return {
                {mv,  0},
                { 0, mv}
        };
}

template <typename T>
T velocity_angle_p(const Vector<2, T>& velocity, const Matrix<2, 2, T>& velocity_p)
{
        // angle = atan(y/x)
        // Jacobian
        //  -y/(x*x+y*y) x/(x*x+y*y)
        const T ns = velocity.norm_squared();
        const T x = velocity[0];
        const T y = velocity[1];
        const Matrix<1, 2, T> error_propagation{
                {-y / ns, x / ns}
        };
        const Matrix<1, 1, T> p = error_propagation * velocity_p * error_propagation.transposed();
        return p(0, 0);
}
}

template <typename T>
PositionFilter<T>::PositionFilter(const T process_variance, const Vector<6, T>& x, const Matrix<6, 6, T>& p)
        : filter_(x, p),
          process_variance_(process_variance)
{
        ASSERT(process_variance >= 0);
        ASSERT(is_finite(x));
        ASSERT(is_finite(p));
}

template <typename T>
void PositionFilter<T>::predict(const T dt)
{
        ASSERT(dt >= 0);
        const Matrix<6, 6, T> f_matrix = f(dt);
        filter_.predict(f_matrix, f_matrix.transposed(), q(dt, process_variance_));
}

template <typename T>
void PositionFilter<T>::update(const Vector<2, T>& position, const T measurement_variance)
{
        ASSERT(measurement_variance >= 0);
        ASSERT(position.is_finite());
        filter_.update(H<T>, H_T<T>, r(measurement_variance), position);
}

template <typename T>
Vector<2, T> PositionFilter<T>::position() const
{
        return {filter_.x()[0], filter_.x()[3]};
}

template <typename T>
Matrix<2, 2, T> PositionFilter<T>::position_p() const
{
        return {
                {filter_.p()(0, 0), filter_.p()(0, 3)},
                {filter_.p()(3, 0), filter_.p()(3, 3)}
        };
}

template <typename T>
Vector<2, T> PositionFilter<T>::velocity() const
{
        return {filter_.x()[1], filter_.x()[4]};
}

template <typename T>
T PositionFilter<T>::angle() const
{
        return std::atan2(filter_.x()[4], filter_.x()[1]);
}

template <typename T>
T PositionFilter<T>::angle_p() const
{
        const Matrix<2, 2, T> velocity_p{
                {filter_.p()(1, 1), filter_.p()(1, 4)},
                {filter_.p()(4, 1), filter_.p()(4, 4)}
        };
        return velocity_angle_p(velocity(), velocity_p);
}

template class PositionFilter<float>;
template class PositionFilter<double>;
template class PositionFilter<long double>;
}
