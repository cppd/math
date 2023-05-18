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

#include "utility.h"

#include "../functions.h"

namespace ns::filter::test
{
namespace
{
struct AddX final
{
        template <typename T>
        [[nodiscard]] Vector<9, T> operator()(const Vector<9, T>& a, const Vector<9, T>& b) const
        {
                Vector<9, T> res = a + b;
                res[6] = normalize_angle(res[6]);
                res[8] = normalize_angle(res[8]);
                return res;
        }
};

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
Matrix<6, 6, T> position_speed_direction_acceleration_r(
        const T position_variance,
        const T speed_variance,
        const T direction_variance,
        const T acceleration_variance)
{
        const T pv = position_variance;
        const T sv = speed_variance;
        const T dv = direction_variance;
        const T av = acceleration_variance;
        return {
                {pv,  0,  0,  0,  0,  0},
                { 0, pv,  0,  0,  0,  0},
                { 0,  0, sv,  0,  0,  0},
                { 0,  0,  0, dv,  0,  0},
                { 0,  0,  0,  0, av,  0},
                { 0,  0,  0,  0,  0, av}
        };
}

template <typename T>
Vector<6, T> position_speed_direction_acceleration_h(const Vector<9, T>& x)
{
        // px = px
        // py = py
        // speed = sqrt(vx*vx + vy*vy)
        // angle = atan(vy, vx) + angle + angle_r
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
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        return {
                px, // px
                py, // py
                std::sqrt(vx * vx + vy * vy), // speed
                std::atan2(vy, vx) + angle + angle_r, // angle
                ax * cos - ay * sin, // ax
                ax * sin + ay * cos // ay
        };
}

template <typename T>
Matrix<6, 9, T> position_speed_direction_acceleration_hj(const Vector<9, T>& x)
{
        // px = px
        // py = py
        // speed = sqrt(vx*vx + vy*vy)
        // angle = atan(vy, vx) + angle + angle_r
        // ax = ax*cos(angle) - ay*sin(angle)
        // ay = ax*sin(angle) + ay*cos(angle)
        // Jacobian
        // mPx=Px;
        // mPy=Py;
        // mSpeed=Sqrt[Vx*Vx+Vy*Vy];
        // mAngle=ArcTan[Vx,Vy]+Bc+Br;
        // mAx=(Ax*Cos[Bc]-Ay*Sin[Bc]);
        // mAy=(Ax*Sin[Bc]+Ay*Cos[Bc]);
        // Simplify[D[{mPx,mPy,mSpeed,mAngle,mAx,mAy},{{Px,Vx,Ax,Py,Vy,Ay,Bc,Bv,Br}}]]
        const T vx = x[1];
        const T ax = x[2];
        const T vy = x[4];
        const T ay = x[5];
        const T angle = x[6];
        const T speed_2 = vx * vx + vy * vy;
        const T speed = std::sqrt(speed_2);
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        const T a_1 = -ax * sin - ay * cos;
        const T a_2 = ax * cos - ay * sin;
        return {
                {1,             0,   0, 0,            0,    0,   0, 0, 0},
                {0,             0,   0, 1,            0,    0,   0, 0, 0},
                {0,    vx / speed,   0, 0,   vy / speed,    0,   0, 0, 0},
                {0, -vy / speed_2,   0, 0, vx / speed_2,    0,   1, 0, 1},
                {0,             0, cos, 0,            0, -sin, a_1, 0, 0},
                {0,             0, sin, 0,            0,  cos, a_2, 0, 0}
        };
}

//

template <typename T>
Matrix<5, 5, T> position_direction_acceleration_r(
        const T position_variance,
        const T direction_variance,
        const T acceleration_variance)
{
        const T pv = position_variance;
        const T dv = direction_variance;
        const T av = acceleration_variance;
        return {
                {pv,  0,  0,  0,  0},
                { 0, pv,  0,  0,  0},
                { 0,  0, dv,  0,  0},
                { 0,  0,  0, av,  0},
                { 0,  0,  0,  0, av}
        };
}

template <typename T>
Vector<5, T> position_direction_acceleration_h(const Vector<9, T>& x)
{
        // px = px
        // py = py
        // angle = atan(vy, vx) + angle + angle_r
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
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        return {
                px, // px
                py, // py
                std::atan2(vy, vx) + angle + angle_r, // angle
                ax * cos - ay * sin, // ax
                ax * sin + ay * cos // ay
        };
}

template <typename T>
Matrix<5, 9, T> position_direction_acceleration_hj(const Vector<9, T>& x)
{
        // px = px
        // py = py
        // angle = atan(vy, vx) + angle + angle_r
        // ax = ax*cos(angle) - ay*sin(angle)
        // ay = ax*sin(angle) + ay*cos(angle)
        // Jacobian
        // mPx=Px;
        // mPy=Py;
        // mAngle=ArcTan[Vx,Vy]+Bc+Br;
        // mAx=(Ax*Cos[Bc]-Ay*Sin[Bc]);
        // mAy=(Ax*Sin[Bc]+Ay*Cos[Bc]);
        // Simplify[D[{mPx,mPy,mAngle,mAx,mAy},{{Px,Vx,Ax,Py,Vy,Ay,Bc,Bv,Br}}]]
        const T vx = x[1];
        const T ax = x[2];
        const T vy = x[4];
        const T ay = x[5];
        const T angle = x[6];
        const T s_2 = vx * vx + vy * vy;
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        const T a_1 = -ax * sin - ay * cos;
        const T a_2 = ax * cos - ay * sin;
        return {
                {1,         0,   0, 0,        0,    0,   0, 0, 0},
                {0,         0,   0, 1,        0,    0,   0, 0, 0},
                {0, -vy / s_2,   0, 0, vx / s_2,    0,   1, 0, 1},
                {0,         0, cos, 0,        0, -sin, a_1, 0, 0},
                {0,         0, sin, 0,        0,  cos, a_2, 0, 0}
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
        // mAx=(Ax*Cos[Bc]-Ay*Sin[Bc]);
        // mAy=(Ax*Sin[Bc]+Ay*Cos[Bc]);
        // Simplify[D[{mAx,mAy},{{Px,Vx,Ax,Py,Vy,Ay,Bc,Bv,Br}}]]
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
        filter_.update(position_h<T>, position_hj<T>, position_r(position_variance), position, AddX(), Subtract());
}

template <typename T>
void ProcessFilterEkf<T>::update_position_speed_direction_acceleration(
        const Vector<2, T>& position,
        const T speed,
        const T direction,
        const Vector<2, T>& acceleration,
        const T position_variance,
        const T speed_variance,
        const T direction_variance,
        const T acceleration_variance)
{
        filter_.update(
                position_speed_direction_acceleration_h<T>, position_speed_direction_acceleration_hj<T>,
                position_speed_direction_acceleration_r(
                        position_variance, speed_variance, direction_variance, acceleration_variance),
                Vector<6, T>(position[0], position[1], speed, direction, acceleration[0], acceleration[1]), AddX(),
                [](const Vector<6, T>& a, const Vector<6, T>& b) -> Vector<6, T>
                {
                        Vector<6, T> res = a - b;
                        res[3] = normalize_angle(res[3]);
                        return res;
                });
}

template <typename T>
void ProcessFilterEkf<T>::update_position_direction_acceleration(
        const Vector<2, T>& position,
        const T direction,
        const Vector<2, T>& acceleration,
        const T position_variance,
        const T direction_variance,
        const T acceleration_variance)
{
        filter_.update(
                position_direction_acceleration_h<T>, position_direction_acceleration_hj<T>,
                position_direction_acceleration_r(position_variance, direction_variance, acceleration_variance),
                Vector<5, T>(position[0], position[1], direction, acceleration[0], acceleration[1]), AddX(),
                [](const Vector<5, T>& a, const Vector<5, T>& b) -> Vector<5, T>
                {
                        Vector<5, T> res = a - b;
                        res[2] = normalize_angle(res[2]);
                        return res;
                });
}

template <typename T>
void ProcessFilterEkf<T>::update_acceleration(const Vector<2, T>& acceleration, const T acceleration_variance)
{
        filter_.update(
                acceleration_h<T>, acceleration_hj<T>, acceleration_r(acceleration_variance), acceleration, AddX(),
                Subtract());
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
