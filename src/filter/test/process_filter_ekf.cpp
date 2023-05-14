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

#include "process_filter_ekf.h"

namespace ns::filter::test
{
namespace
{
template <typename T>
Matrix<9, 9, T> f(const T dt)
{
        const T dt_2 = square(dt) / 2;
        return {
                {1, dt, dt_2, 0,  0,    0, 0,  0, 0},
                {0,  1,   dt, 0,  0,    0, 0,  0, 0},
                {0,  0,    1, 0,  0,    0, 0,  0, 0},
                {0,  0,    0, 1, dt, dt_2, 0,  0, 0},
                {0,  0,    0, 0,  1,   dt, 0,  0, 0},
                {0,  0,    0, 0,  0,    1, 0,  0, 0},
                {0,  0,    0, 0,  0,    0, 1, dt, 0},
                {0,  0,    0, 0,  0,    0, 0,  1, 0},
                {0,  0,    0, 0,  0,    0, 0,  0, 1}
        };
}

template <typename T>
constexpr Matrix<9, 9, T> q(const T dt, const T position_variance, const T angle_variance, const T angle_r_variance)
{
        const T dt_2 = square(dt) / 2;
        const Matrix<9, 4, T> noise_transition{
                {dt_2,    0,    0, 0},
                {  dt,    0,    0, 0},
                {   1,    0,    0, 0},
                {   0, dt_2,    0, 0},
                {   0,   dt,    0, 0},
                {   0,    1,    0, 0},
                {   0,    0, dt_2, 0},
                {   0,    0,   dt, 0},
                {   0,    0,    0, 1}
        };

        const T p = position_variance;
        const T a = angle_variance;
        const T a_r = angle_r_variance;
        const Matrix<4, 4, T> process_covariance{
                {p, 0, 0,   0},
                {0, p, 0,   0},
                {0, 0, a,   0},
                {0, 0, 0, a_r}
        };

        return noise_transition * process_covariance * noise_transition.transposed();
}

//

template <typename T>
Matrix<2, 2, T> position_r(const T position_variance)
{
        const T pv = position_variance;
        return {
                {pv,  0},
                { 0, pv}
        };
}

template <typename T>
Vector<2, T> position_h(const Vector<9, T>& x)
{
        // px = px
        // py = py
        return {x[0], x[3]};
}

template <typename T>
Matrix<2, 9, T> position_hj(const Vector<9, T>& /*x*/)
{
        // px = px
        // py = py
        // Jacobian
        return {
                {1, 0, 0, 0, 0, 0, 0, 0, 0},
                {0, 0, 0, 1, 0, 0, 0, 0, 0}
        };
}

//

template <typename T>
Matrix<6, 6, T> position_velocity_acceleration_r(
        const Vector<2, T>& direction,
        const T speed,
        const T position_variance,
        const T speed_variance,
        const T direction_variance,
        const T acceleration_variance)
{
        const T pv = position_variance;
        const T sv = speed_variance;
        const T dv = direction_variance;
        const T av = acceleration_variance;
        const Matrix<6, 6, T> r{
                {pv,  0,  0,  0,  0,  0},
                { 0, pv,  0,  0,  0,  0},
                { 0,  0, sv,  0,  0,  0},
                { 0,  0,  0, dv,  0,  0},
                { 0,  0,  0,  0, av,  0},
                { 0,  0,  0,  0,  0, av}
        };

        // px = px
        // py = py
        // vx = speed*cos(angle)
        // vy = speed*sin(angle)
        // ax = ax
        // ay = ay
        // Jacobian
        const T cos = direction[0];
        const T sin = direction[1];
        const Matrix<6, 6, T> error_propagation{
                {1, 0,   0,            0, 0, 0},
                {0, 1,   0,            0, 0, 0},
                {0, 0, cos, -speed * sin, 0, 0},
                {0, 0, sin,  speed * cos, 0, 0},
                {0, 0,   0,            0, 1, 0},
                {0, 0,   0,            0, 0, 1}
        };

        return error_propagation * r * error_propagation.transposed();
}

template <typename T>
Vector<6, T> position_velocity_acceleration_h(const Vector<9, T>& x)
{
        // px = px
        // py = py
        // dx = vx*cos(angle + angle_r) - vy*sin(angle + angle_r)
        // dy = vx*sin(angle + angle_r) + vy*cos(angle + angle_r)
        // ax = ax*cos(angle) - ay*sin(angle)
        // ay = ax*sin(angle) + ay*cos(angle)
        const T px = x[0];
        const T vx = x[1];
        const T ax = x[2];
        const T py = x[3];
        const T vy = x[4];
        const T ay = x[5];
        const T angle = x[6];
        const T angle_r = x[8];
        const T cos_v = std::cos(angle + angle_r);
        const T sin_v = std::sin(angle + angle_r);
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        return {px, py, vx * cos_v - vy * sin_v, vx * sin_v + vy * cos_v, ax * cos - ay * sin, ax * sin + ay * cos};
}

template <typename T>
Matrix<6, 9, T> position_velocity_acceleration_hj(const Vector<9, T>& x)
{
        // px = px
        // py = py
        // dx = vx*cos(angle + angle_r) - vy*sin(angle + angle_r)
        // dy = vx*sin(angle + angle_r) + vy*cos(angle + angle_r)
        // ax = ax*cos(angle) - ay*sin(angle)
        // ay = ax*sin(angle) + ay*cos(angle)
        // Jacobian
        const T vx = x[1];
        const T vy = x[4];
        const T ax = x[2];
        const T ay = x[5];
        const T angle = x[6];
        const T angle_r = x[8];
        const T cos_v = std::cos(angle + angle_r);
        const T sin_v = std::sin(angle + angle_r);
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        const T d_1 = -vx * sin_v - vy * cos_v;
        const T d_2 = vx * cos_v - vy * sin_v;
        const T a_1 = -ax * sin - ay * cos;
        const T a_2 = ax * cos - ay * sin;
        return {
                {1,     0,   0, 0,      0,    0,   0, 0,   0},
                {0,     0,   0, 1,      0,    0,   0, 0,   0},
                {0, cos_v,   0, 0, -sin_v,    0, d_1, 0, d_1},
                {0, sin_v,   0, 0,  cos_v,    0, d_2, 0, d_2},
                {0,     0, cos, 0,      0, -sin, a_1, 0,   0},
                {0,     0, sin, 0,      0,  cos, a_2, 0,   0}
        };
}

//

template <typename T>
Matrix<6, 6, T> position_direction_acceleration_r(
        const Vector<2, T>& direction,
        const T position_variance,
        const T direction_variance,
        const T acceleration_variance)
{
        const T pv = position_variance;
        const T dv = direction_variance;
        const T av = acceleration_variance;
        const Matrix<5, 5, T> r{
                {pv,  0,  0,  0,  0},
                { 0, pv,  0,  0,  0},
                { 0,  0, dv,  0,  0},
                { 0,  0,  0, av,  0},
                { 0,  0,  0,  0, av}
        };

        // dx = cos(angle)
        // dy = sin(angle)
        // ax = ax
        // ay = ay
        // Jacobian
        const T cos = direction[0];
        const T sin = direction[1];
        const Matrix<6, 5, T> error_propagation{
                {1, 0,    0, 0, 0},
                {0, 1,    0, 0, 0},
                {0, 0, -sin, 0, 0},
                {0, 0,  cos, 0, 0},
                {0, 0,    0, 1, 0},
                {0, 0,    0, 0, 1}
        };

        return error_propagation * r * error_propagation.transposed();
}

template <typename T>
Vector<6, T> position_direction_acceleration_h(const Vector<9, T>& x)
{
        // px = px
        // py = py
        // dx = (vx*cos(angle + angle_r) - vy*sin(angle + angle_r)) / sqrt(vx*vx + vy*vy);
        // dy = (vx*sin(angle + angle_r) + vy*cos(angle + angle_r)) / sqrt(vx*vx + vy*vy);
        // ax = (ax*cos(angle) - ay*sin(angle))
        // ay = (ax*sin(angle) + ay*cos(angle))
        const T px = x[0];
        const T vx = x[1];
        const T ax = x[2];
        const T py = x[3];
        const T vy = x[4];
        const T ay = x[5];
        const T angle = x[6];
        const T angle_r = x[8];
        const T speed = std::sqrt(square(vx) + square(vy));
        const T cos_v = std::cos(angle + angle_r);
        const T sin_v = std::sin(angle + angle_r);
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        return {px,
                py,
                (vx * cos_v - vy * sin_v) / speed,
                (vx * sin_v + vy * cos_v) / speed,
                ax * cos - ay * sin,
                ax * sin + ay * cos};
}

template <typename T>
Matrix<6, 9, T> position_direction_acceleration_hj(const Vector<9, T>& x)
{
        // px = px
        // py = py
        // dx = (vx*cos(angle + angle_r) - vy*sin(angle + angle_r)) / sqrt(vx*vx + vy*vy);
        // dy = (vx*sin(angle + angle_r) + vy*cos(angle + angle_r)) / sqrt(vx*vx + vy*vy);
        // ax = (ax*cos(angle) - ay*sin(angle))
        // ay = (ax*sin(angle) + ay*cos(angle))
        // Jacobian
        // mPx=Px;
        // mPy=Py;
        // mDx=(Vx*Cos[Angle+AngleR]-Vy*Sin[Angle+AngleR])/Sqrt[Vx*Vx+Vy*Vy];
        // mDy=(Vx*Sin[Angle+AngleR]+Vy*Cos[Angle+AngleR])/Sqrt[Vx*Vx+Vy*Vy];
        // mAx=(Ax*Cos[Angle]-Ay*Sin[Angle]);
        // mAy=(Ax*Sin[Angle]+Ay*Cos[Angle]);
        // Simplify[D[{mPx,mPy,mDx,mDy,mAx,mAy},{{Px,Vx,Ax,Py,Vy,Ay,Angle,AngleV,AngleR}}]]
        const T vx = x[1];
        const T vy = x[4];
        const T ax = x[2];
        const T ay = x[5];
        const T angle = x[6];
        const T angle_r = x[8];
        const T l = std::sqrt(square(vx) + square(vy));
        const T l_3 = power<3>(l);
        const T cos_v = std::cos(angle + angle_r);
        const T sin_v = std::sin(angle + angle_r);
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        const T d_1 = vy * cos_v + vx * sin_v;
        const T d_2 = vx * cos_v - vy * sin_v;
        const T a_1 = -ax * sin - ay * cos;
        const T a_2 = ax * cos - ay * sin;
        return {
                {1,               0,   0, 0,               0,    0,        0, 0,        0},
                {0,               0,   0, 1,               0,    0,        0, 0,        0},
                {0,  vy * d_1 / l_3,   0, 0, -vx * d_1 / l_3,    0, -d_1 / l, 0, -d_1 / l},
                {0, -vy * d_2 / l_3,   0, 0,  vx * d_2 / l_3,    0,  d_2 / l, 0,  d_2 / l},
                {0,               0, cos, 0,               0, -sin,      a_1, 0,        0},
                {0,               0, sin, 0,               0,  cos,      a_2, 0,        0}
        };
}

//

template <typename T>
Matrix<2, 2, T> acceleration_r(const T acceleration_variance)
{
        const T av = acceleration_variance;
        return {
                {av,  0},
                { 0, av}
        };
}

template <typename T>
Vector<2, T> acceleration_h(const Vector<9, T>& x)
{
        // ax = ax*cos(angle) - ay*sin(angle)
        // ay = ax*sin(angle) + ay*cos(angle)
        const T ax = x[2];
        const T ay = x[5];
        const T angle = x[6];
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        return {ax * cos - ay * sin, ax * sin + ay * cos};
}

template <typename T>
Matrix<2, 9, T> acceleration_hj(const Vector<9, T>& x)
{
        // ax = ax*cos(angle) - ay*sin(angle)
        // ay = ax*sin(angle) + ay*cos(angle)
        // Jacobian
        const T ax = x[2];
        const T ay = x[5];
        const T angle = x[6];
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        return {
                {0, 0, cos, 0, 0, -sin, -ax * sin - ay * cos, 0, 0},
                {0, 0, sin, 0, 0,  cos,  ax * cos - ay * sin, 0, 0}
        };
}
}

template <typename T>
ProcessFilterEkf<T>::ProcessFilterEkf(
        const T dt,
        const T position_variance,
        const T angle_variance,
        const T angle_r_variance,
        const Vector<9, T>& x,
        const Matrix<9, 9, T>& p)
        : filter_(x, p),
          f_(f(dt)),
          f_t_(f_.transposed()),
          q_(q(dt, position_variance, angle_variance, angle_r_variance))
{
}

template <typename T>
void ProcessFilterEkf<T>::predict()
{
        filter_.predict(f_, f_t_, q_);
}

template <typename T>
void ProcessFilterEkf<T>::update_position(const Vector<2, T>& position, const T position_variance)
{
        filter_.update(position_h<T>, position_hj<T>, position_r(position_variance), position);
}

template <typename T>
void ProcessFilterEkf<T>::update_position_velocity_acceleration(
        const Vector<2, T>& position,
        const T speed,
        const Vector<2, T>& direction,
        const Vector<2, T>& acceleration,
        const T position_variance,
        const T speed_variance,
        const T direction_variance,
        const T acceleration_variance)
{
        filter_.update(
                position_velocity_acceleration_h<T>, position_velocity_acceleration_hj<T>,
                position_velocity_acceleration_r(
                        direction, speed, position_variance, speed_variance, direction_variance, acceleration_variance),
                Vector<6, T>(
                        position[0], position[1], direction[0] * speed, direction[1] * speed, acceleration[0],
                        acceleration[1]));
}

template <typename T>
void ProcessFilterEkf<T>::update_position_direction_acceleration(
        const Vector<2, T>& position,
        const Vector<2, T>& direction,
        const Vector<2, T>& acceleration,
        const T position_variance,
        const T direction_variance,
        const T acceleration_variance)
{
        filter_.update(
                position_direction_acceleration_h<T>, position_direction_acceleration_hj<T>,
                position_direction_acceleration_r(
                        direction, position_variance, direction_variance, acceleration_variance),
                Vector<6, T>(position[0], position[1], direction[0], direction[1], acceleration[0], acceleration[1]));
}

template <typename T>
void ProcessFilterEkf<T>::update_acceleration(const Vector<2, T>& acceleration, const T acceleration_variance)
{
        filter_.update(acceleration_h<T>, acceleration_hj<T>, acceleration_r(acceleration_variance), acceleration);
}

template <typename T>
Vector<2, T> ProcessFilterEkf<T>::position() const
{
        return {filter_.x()[0], filter_.x()[3]};
}

template <typename T>
Matrix<2, 2, T> ProcessFilterEkf<T>::position_p() const
{
        return {
                {filter_.p()(0, 0), filter_.p()(0, 3)},
                {filter_.p()(3, 0), filter_.p()(3, 3)}
        };
}

template <typename T>
T ProcessFilterEkf<T>::speed() const
{
        return Vector<2, T>(filter_.x()[1], filter_.x()[4]).norm();
}

template <typename T>
T ProcessFilterEkf<T>::angle() const
{
        return filter_.x()[6];
}

template <typename T>
T ProcessFilterEkf<T>::angle_speed() const
{
        return filter_.x()[7];
}

template <typename T>
T ProcessFilterEkf<T>::angle_p() const
{
        return filter_.p()(6, 6);
}

template <typename T>
T ProcessFilterEkf<T>::angle_r() const
{
        return filter_.x()[8];
}

template <typename T>
T ProcessFilterEkf<T>::angle_r_p() const
{
        return filter_.p()(8, 8);
}

template class ProcessFilterEkf<float>;
template class ProcessFilterEkf<double>;
template class ProcessFilterEkf<long double>;
}
