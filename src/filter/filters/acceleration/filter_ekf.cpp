/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "filter_ekf.h"

#include "init.h"

#include <src/com/angle.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/filter/core/ekf.h>
#include <src/filter/filters/com/utility.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/utility/instantiation.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <memory>
#include <optional>

namespace ns::filter::filters::acceleration
{
namespace
{
template <typename T>
constexpr std::optional<T> THETA{};

constexpr bool NORMALIZED_INNOVATION{false};
constexpr bool LIKELIHOOD{false};

template <typename T>
numerical::Vector<9, T> x(const numerical::Vector<4, T>& position_velocity, const Init<T>& init)
{
        ASSERT(is_finite(position_velocity));

        numerical::Vector<9, T> res;

        res[0] = position_velocity[0];
        res[1] = position_velocity[1];
        res[2] = init.acceleration;
        res[3] = position_velocity[2];
        res[4] = position_velocity[3];
        res[5] = init.acceleration;
        res[6] = init.angle;
        res[7] = init.angle_speed;
        res[8] = init.angle_r;

        return res;
}

template <typename T>
numerical::Matrix<9, 9, T> p(const numerical::Matrix<4, 4, T>& position_velocity_p, const Init<T>& init)
{
        ASSERT(is_finite(position_velocity_p));

        const numerical::Matrix<4, 4, T>& p = position_velocity_p;
        static constexpr std::size_t N = 2;

        numerical::Matrix<9, 9, T> res(0);

        for (std::size_t r = 0; r < N; ++r)
        {
                for (std::size_t i = 0; i < 2; ++i)
                {
                        for (std::size_t c = 0; c < N; ++c)
                        {
                                for (std::size_t j = 0; j < 2; ++j)
                                {
                                        res[3 * r + i, 3 * c + j] = p[2 * r + i, 2 * c + j];
                                }
                        }
                }
        }

        res[2, 2] = init.acceleration_variance;
        res[5, 5] = init.acceleration_variance;
        res[6, 6] = init.angle_variance;
        res[7, 7] = init.angle_speed_variance;
        res[8, 8] = init.angle_r_variance;

        return res;
}

template <typename T>
[[nodiscard]] numerical::Vector<9, T> add_x(const numerical::Vector<9, T>& a, const numerical::Vector<9, T>& b)
{
        numerical::Vector<9, T> res = a + b;
        res[6] = normalize_angle(res[6]);
        res[8] = normalize_angle(res[8]);
        return res;
}

template <typename T>
numerical::Matrix<9, 9, T> f(const T dt)
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
constexpr numerical::Matrix<9, 9, T> q(
        const T dt,
        const T position_variance,
        const T angle_variance,
        const T angle_r_variance)
{
        const T dt_2 = power<2>(dt) / 2;
        const T dt_3 = power<3>(dt) / 6;
        const numerical::Matrix<9, 4, T> noise_transition{
                {dt_3,    0,    0,  0},
                {dt_2,    0,    0,  0},
                {  dt,    0,    0,  0},
                {   0, dt_3,    0,  0},
                {   0, dt_2,    0,  0},
                {   0,   dt,    0,  0},
                {   0,    0, dt_2,  0},
                {   0,    0,   dt,  0},
                {   0,    0,    0, dt}
        };

        const T p = position_variance;
        const T a = angle_variance;
        const T a_r = angle_r_variance;
        const numerical::Matrix<4, 4, T> covariance{
                {p, 0, 0,   0},
                {0, p, 0,   0},
                {0, 0, a,   0},
                {0, 0, 0, a_r}
        };

        return noise_transition * covariance * noise_transition.transposed();
}

//

template <typename T>
numerical::Matrix<2, 2, T> position_r(const numerical::Vector<2, T>& position_variance)
{
        return numerical::make_diagonal_matrix(position_variance);
}

template <typename T>
numerical::Vector<2, T> position_h(const numerical::Vector<9, T>& x)
{
        // px = px
        // py = py
        return {x[0], x[3]};
}

template <typename T>
numerical::Matrix<2, 9, T> position_hj(const numerical::Vector<9, T>& /*x*/)
{
        // px = px
        // py = py
        // Jacobian
        return {
                {1, 0, 0, 0, 0, 0, 0, 0, 0},
                {0, 0, 0, 1, 0, 0, 0, 0, 0}
        };
}

template <typename T>
numerical::Vector<2, T> position_residual(const numerical::Vector<2, T>& a, const numerical::Vector<2, T>& b)
{
        return a - b;
}

//

template <typename T>
numerical::Matrix<3, 3, T> position_speed_r(
        const numerical::Vector<2, T>& position_variance,
        const numerical::Vector<1, T>& speed_variance)
{
        const numerical::Vector<2, T>& pv = position_variance;
        const numerical::Vector<1, T>& sv = speed_variance;
        return numerical::make_diagonal_matrix<3, T>({pv[0], pv[1], sv[0]});
}

template <typename T>
numerical::Vector<3, T> position_speed_h(const numerical::Vector<9, T>& x)
{
        // px = px
        // py = py
        // speed = sqrt(vx*vx + vy*vy)
        const T px = x[0];
        const T vx = x[1];
        const T py = x[3];
        const T vy = x[4];
        return {
                px, // px
                py, // py
                std::sqrt(vx * vx + vy * vy), // speed
        };
}

template <typename T>
numerical::Matrix<3, 9, T> position_speed_hj(const numerical::Vector<9, T>& x)
{
        // px = px
        // py = py
        // speed = sqrt(vx*vx + vy*vy)
        // Jacobian
        // mPx=Px;
        // mPy=Py;
        // mSpeed=Sqrt[Vx*Vx+Vy*Vy];
        // Simplify[D[{mPx,mPy,mSpeed},{{Px,Vx,Ax,Py,Vy,Ay,Bc,Bv,Br}}]]
        const T vx = x[1];
        const T vy = x[4];
        const T speed = std::sqrt(vx * vx + vy * vy);
        return {
                {1,          0, 0, 0,          0, 0, 0, 0, 0},
                {0,          0, 0, 1,          0, 0, 0, 0, 0},
                {0, vx / speed, 0, 0, vy / speed, 0, 0, 0, 0}
        };
}

template <typename T>
numerical::Vector<3, T> position_speed_residual(const numerical::Vector<3, T>& a, const numerical::Vector<3, T>& b)
{
        return a - b;
}

//

template <typename T>
numerical::Matrix<6, 6, T> position_speed_direction_acceleration_r(
        const numerical::Vector<2, T>& position_variance,
        const numerical::Vector<1, T>& speed_variance,
        const numerical::Vector<1, T>& direction_variance,
        const numerical::Vector<2, T>& acceleration_variance)
{
        const numerical::Vector<2, T>& pv = position_variance;
        const numerical::Vector<1, T>& sv = speed_variance;
        const numerical::Vector<1, T>& dv = direction_variance;
        const numerical::Vector<2, T>& av = acceleration_variance;
        return numerical::make_diagonal_matrix<6, T>({pv[0], pv[1], sv[0], dv[0], av[0], av[1]});
}

template <typename T>
numerical::Vector<6, T> position_speed_direction_acceleration_h(const numerical::Vector<9, T>& x)
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
numerical::Matrix<6, 9, T> position_speed_direction_acceleration_hj(const numerical::Vector<9, T>& x)
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

template <typename T>
numerical::Vector<6, T> position_speed_direction_acceleration_residual(
        const numerical::Vector<6, T>& a,
        const numerical::Vector<6, T>& b)
{
        numerical::Vector<6, T> res = a - b;
        res[3] = normalize_angle(res[3]);
        return res;
}

//

template <typename T>
numerical::Matrix<4, 4, T> position_speed_direction_r(
        const numerical::Vector<2, T>& position_variance,
        const numerical::Vector<1, T>& speed_variance,
        const numerical::Vector<1, T>& direction_variance)
{
        const numerical::Vector<2, T>& pv = position_variance;
        const numerical::Vector<1, T>& sv = speed_variance;
        const numerical::Vector<1, T>& dv = direction_variance;
        return numerical::make_diagonal_matrix<4, T>({pv[0], pv[1], sv[0], dv[0]});
}

template <typename T>
numerical::Vector<4, T> position_speed_direction_h(const numerical::Vector<9, T>& x)
{
        // px = px
        // py = py
        // speed = sqrt(vx*vx + vy*vy)
        // angle = atan(vy, vx) + angle + angle_r
        const T px = x[0];
        const T vx = x[1];
        const T py = x[3];
        const T vy = x[4];
        const T angle = x[6];
        const T angle_r = x[8];
        return {
                px, // px
                py, // py
                std::sqrt(vx * vx + vy * vy), // speed
                std::atan2(vy, vx) + angle + angle_r // angle
        };
}

template <typename T>
numerical::Matrix<4, 9, T> position_speed_direction_hj(const numerical::Vector<9, T>& x)
{
        // px = px
        // py = py
        // speed = sqrt(vx*vx + vy*vy)
        // angle = atan(vy, vx) + angle + angle_r
        // Jacobian
        // mPx=Px;
        // mPy=Py;
        // mSpeed=Sqrt[Vx*Vx+Vy*Vy];
        // mAngle=ArcTan[Vx,Vy]+Bc+Br;
        // Simplify[D[{mPx,mPy,mSpeed,mAngle},{{Px,Vx,Ax,Py,Vy,Ay,Bc,Bv,Br}}]]
        const T vx = x[1];
        const T vy = x[4];
        const T speed_2 = vx * vx + vy * vy;
        const T speed = std::sqrt(speed_2);
        return {
                {1,             0, 0, 0,            0, 0, 0, 0, 0},
                {0,             0, 0, 1,            0, 0, 0, 0, 0},
                {0,    vx / speed, 0, 0,   vy / speed, 0, 0, 0, 0},
                {0, -vy / speed_2, 0, 0, vx / speed_2, 0, 1, 0, 1}
        };
}

template <typename T>
numerical::Vector<4, T> position_speed_direction_residual(
        const numerical::Vector<4, T>& a,
        const numerical::Vector<4, T>& b)
{
        numerical::Vector<4, T> res = a - b;
        res[3] = normalize_angle(res[3]);
        return res;
}

//

template <typename T>
numerical::Matrix<5, 5, T> position_speed_acceleration_r(
        const numerical::Vector<2, T>& position_variance,
        const numerical::Vector<1, T>& speed_variance,
        const numerical::Vector<2, T>& acceleration_variance)
{
        const numerical::Vector<2, T>& pv = position_variance;
        const numerical::Vector<1, T>& sv = speed_variance;
        const numerical::Vector<2, T>& av = acceleration_variance;
        return numerical::make_diagonal_matrix<5, T>({pv[0], pv[1], sv[0], av[0], av[1]});
}

template <typename T>
numerical::Vector<5, T> position_speed_acceleration_h(const numerical::Vector<9, T>& x)
{
        // px = px
        // py = py
        // speed = sqrt(vx*vx + vy*vy)
        // ax = ax*cos(angle) - ay*sin(angle)
        // ay = ax*sin(angle) + ay*cos(angle)
        const T px = x[0];
        const T vx = x[1];
        const T ax = x[2];
        const T py = x[3];
        const T vy = x[4];
        const T ay = x[5];
        const T angle = x[6];
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        return {
                px, // px
                py, // py
                std::sqrt(vx * vx + vy * vy), // speed
                ax * cos - ay * sin, // ax
                ax * sin + ay * cos // ay
        };
}

template <typename T>
numerical::Matrix<5, 9, T> position_speed_acceleration_hj(const numerical::Vector<9, T>& x)
{
        // px = px
        // py = py
        // speed = sqrt(vx*vx + vy*vy)
        // ax = ax*cos(angle) - ay*sin(angle)
        // ay = ax*sin(angle) + ay*cos(angle)
        // Jacobian
        // mPx=Px;
        // mPy=Py;
        // mSpeed=Sqrt[Vx*Vx+Vy*Vy];
        // mAx=(Ax*Cos[Bc]-Ay*Sin[Bc]);
        // mAy=(Ax*Sin[Bc]+Ay*Cos[Bc]);
        // Simplify[D[{mPx,mPy,mSpeed,mAx,mAy},{{Px,Vx,Ax,Py,Vy,Ay,Bc,Bv,Br}}]]
        const T vx = x[1];
        const T ax = x[2];
        const T vy = x[4];
        const T ay = x[5];
        const T angle = x[6];
        const T speed = std::sqrt(vx * vx + vy * vy);
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        const T a_1 = -ax * sin - ay * cos;
        const T a_2 = ax * cos - ay * sin;
        return {
                {1,          0,   0, 0,          0,    0,   0, 0, 0},
                {0,          0,   0, 1,          0,    0,   0, 0, 0},
                {0, vx / speed,   0, 0, vy / speed,    0,   0, 0, 0},
                {0,          0, cos, 0,          0, -sin, a_1, 0, 0},
                {0,          0, sin, 0,          0,  cos, a_2, 0, 0}
        };
}

template <typename T>
numerical::Vector<5, T> position_speed_acceleration_residual(
        const numerical::Vector<5, T>& a,
        const numerical::Vector<5, T>& b)
{
        return a - b;
}

//

template <typename T>
numerical::Matrix<5, 5, T> position_direction_acceleration_r(
        const numerical::Vector<2, T>& position_variance,
        const numerical::Vector<1, T>& direction_variance,
        const numerical::Vector<2, T>& acceleration_variance)
{
        const numerical::Vector<2, T>& pv = position_variance;
        const numerical::Vector<1, T>& dv = direction_variance;
        const numerical::Vector<2, T>& av = acceleration_variance;
        return numerical::make_diagonal_matrix<5, T>({pv[0], pv[1], dv[0], av[0], av[1]});
}

template <typename T>
numerical::Vector<5, T> position_direction_acceleration_h(const numerical::Vector<9, T>& x)
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
numerical::Matrix<5, 9, T> position_direction_acceleration_hj(const numerical::Vector<9, T>& x)
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

template <typename T>
numerical::Vector<5, T> position_direction_acceleration_residual(
        const numerical::Vector<5, T>& a,
        const numerical::Vector<5, T>& b)
{
        numerical::Vector<5, T> res = a - b;
        res[2] = normalize_angle(res[2]);
        return res;
}

//

template <typename T>
numerical::Matrix<3, 3, T> position_direction_r(
        const numerical::Vector<2, T>& position_variance,
        const numerical::Vector<1, T>& direction_variance)
{
        const numerical::Vector<2, T>& pv = position_variance;
        const numerical::Vector<1, T>& dv = direction_variance;
        return numerical::make_diagonal_matrix<3, T>({pv[0], pv[1], dv[0]});
}

template <typename T>
numerical::Vector<3, T> position_direction_h(const numerical::Vector<9, T>& x)
{
        // px = px
        // py = py
        // angle = atan(vy, vx) + angle + angle_r
        const T px = x[0];
        const T vx = x[1];
        const T py = x[3];
        const T vy = x[4];
        const T angle = x[6];
        const T angle_r = x[8];
        return {
                px, // px
                py, // py
                std::atan2(vy, vx) + angle + angle_r // angle
        };
}

template <typename T>
numerical::Matrix<3, 9, T> position_direction_hj(const numerical::Vector<9, T>& x)
{
        // px = px
        // py = py
        // angle = atan(vy, vx) + angle + angle_r
        // Jacobian
        // mPx=Px;
        // mPy=Py;
        // mAngle=ArcTan[Vx,Vy]+Bc+Br;
        // Simplify[D[{mPx,mPy,mAngle},{{Px,Vx,Ax,Py,Vy,Ay,Bc,Bv,Br}}]]
        const T vx = x[1];
        const T vy = x[4];
        const T s_2 = vx * vx + vy * vy;
        return {
                {1,         0, 0, 0,        0, 0, 0, 0, 0},
                {0,         0, 0, 1,        0, 0, 0, 0, 0},
                {0, -vy / s_2, 0, 0, vx / s_2, 0, 1, 0, 1}
        };
}

template <typename T>
numerical::Vector<3, T> position_direction_residual(const numerical::Vector<3, T>& a, const numerical::Vector<3, T>& b)
{
        numerical::Vector<3, T> res = a - b;
        res[2] = normalize_angle(res[2]);
        return res;
}

//

template <typename T>
numerical::Matrix<4, 4, T> position_acceleration_r(
        const numerical::Vector<2, T>& position_variance,
        const numerical::Vector<2, T>& acceleration_variance)
{
        const numerical::Vector<2, T>& pv = position_variance;
        const numerical::Vector<2, T>& av = acceleration_variance;
        return numerical::make_diagonal_matrix<4, T>({pv[0], pv[1], av[0], av[1]});
}

template <typename T>
numerical::Vector<4, T> position_acceleration_h(const numerical::Vector<9, T>& x)
{
        // px = px
        // py = py
        // ax = ax*cos(angle) - ay*sin(angle)
        // ay = ax*sin(angle) + ay*cos(angle)
        const T px = x[0];
        const T ax = x[2];
        const T py = x[3];
        const T ay = x[5];
        const T angle = x[6];
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        return {
                px, // px
                py, // py
                ax * cos - ay * sin, // ax
                ax * sin + ay * cos // ay
        };
}

template <typename T>
numerical::Matrix<4, 9, T> position_acceleration_hj(const numerical::Vector<9, T>& x)
{
        // px = px
        // py = py
        // ax = ax*cos(angle) - ay*sin(angle)
        // ay = ax*sin(angle) + ay*cos(angle)
        // Jacobian
        // mPx=Px;
        // mPy=Py;
        // mAx=(Ax*Cos[Bc]-Ay*Sin[Bc]);
        // mAy=(Ax*Sin[Bc]+Ay*Cos[Bc]);
        // Simplify[D[{mPx,mPy,mAx,mAy},{{Px,Vx,Ax,Py,Vy,Ay,Bc,Bv,Br}}]]
        const T ax = x[2];
        const T ay = x[5];
        const T angle = x[6];
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        const T a_1 = -ax * sin - ay * cos;
        const T a_2 = ax * cos - ay * sin;
        return {
                {1, 0,   0, 0, 0,    0,   0, 0, 0},
                {0, 0,   0, 1, 0,    0,   0, 0, 0},
                {0, 0, cos, 0, 0, -sin, a_1, 0, 0},
                {0, 0, sin, 0, 0,  cos, a_2, 0, 0}
        };
}

template <typename T>
numerical::Vector<4, T> position_acceleration_residual(
        const numerical::Vector<4, T>& a,
        const numerical::Vector<4, T>& b)
{
        return a - b;
}

//

template <typename T>
numerical::Matrix<4, 4, T> speed_direction_acceleration_r(
        const numerical::Vector<1, T>& speed_variance,
        const numerical::Vector<1, T>& direction_variance,
        const numerical::Vector<2, T>& acceleration_variance)
{
        const numerical::Vector<1, T>& sv = speed_variance;
        const numerical::Vector<1, T>& dv = direction_variance;
        const numerical::Vector<2, T>& av = acceleration_variance;
        return numerical::make_diagonal_matrix<4, T>({sv[0], dv[0], av[0], av[1]});
}

template <typename T>
numerical::Vector<4, T> speed_direction_acceleration_h(const numerical::Vector<9, T>& x)
{
        // speed = sqrt(vx*vx + vy*vy)
        // angle = atan(vy, vx) + angle + angle_r
        // ax = ax*cos(angle) - ay*sin(angle)
        // ay = ax*sin(angle) + ay*cos(angle)
        const T vx = x[1];
        const T ax = x[2];
        const T vy = x[4];
        const T ay = x[5];
        const T angle = x[6];
        const T angle_r = x[8];
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        return {
                std::sqrt(vx * vx + vy * vy), // speed
                std::atan2(vy, vx) + angle + angle_r, // angle
                ax * cos - ay * sin, // ax
                ax * sin + ay * cos // ay
        };
}

template <typename T>
numerical::Matrix<4, 9, T> speed_direction_acceleration_hj(const numerical::Vector<9, T>& x)
{
        // speed = sqrt(vx*vx + vy*vy)
        // angle = atan(vy, vx) + angle + angle_r
        // ax = ax*cos(angle) - ay*sin(angle)
        // ay = ax*sin(angle) + ay*cos(angle)
        // Jacobian
        // mSpeed=Sqrt[Vx*Vx+Vy*Vy];
        // mAngle=ArcTan[Vx,Vy]+Bc+Br;
        // mAx=(Ax*Cos[Bc]-Ay*Sin[Bc]);
        // mAy=(Ax*Sin[Bc]+Ay*Cos[Bc]);
        // Simplify[D[{mSpeed,mAngle,mAx,mAy},{{Px,Vx,Ax,Py,Vy,Ay,Bc,Bv,Br}}]]
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
                {0,    vx / speed,   0, 0,   vy / speed,    0,   0, 0, 0},
                {0, -vy / speed_2,   0, 0, vx / speed_2,    0,   1, 0, 1},
                {0,             0, cos, 0,            0, -sin, a_1, 0, 0},
                {0,             0, sin, 0,            0,  cos, a_2, 0, 0}
        };
}

template <typename T>
numerical::Vector<4, T> speed_direction_acceleration_residual(
        const numerical::Vector<4, T>& a,
        const numerical::Vector<4, T>& b)
{
        numerical::Vector<4, T> res = a - b;
        res[1] = normalize_angle(res[1]);
        return res;
}

//

template <typename T>
numerical::Matrix<2, 2, T> speed_direction_r(
        const numerical::Vector<1, T>& speed_variance,
        const numerical::Vector<1, T>& direction_variance)
{
        const numerical::Vector<1, T>& sv = speed_variance;
        const numerical::Vector<1, T>& dv = direction_variance;
        return numerical::make_diagonal_matrix<2, T>({sv[0], dv[0]});
}

template <typename T>
numerical::Vector<2, T> speed_direction_h(const numerical::Vector<9, T>& x)
{
        // speed = sqrt(vx*vx + vy*vy)
        // angle = atan(vy, vx) + angle + angle_r
        const T vx = x[1];
        const T vy = x[4];
        const T angle = x[6];
        const T angle_r = x[8];
        return {
                std::sqrt(vx * vx + vy * vy), // speed
                std::atan2(vy, vx) + angle + angle_r // angle
        };
}

template <typename T>
numerical::Matrix<2, 9, T> speed_direction_hj(const numerical::Vector<9, T>& x)
{
        // speed = sqrt(vx*vx + vy*vy)
        // angle = atan(vy, vx) + angle + angle_r
        // Jacobian
        // mSpeed=Sqrt[Vx*Vx+Vy*Vy];
        // mAngle=ArcTan[Vx,Vy]+Bc+Br;
        // Simplify[D[{mSpeed,mAngle},{{Px,Vx,Ax,Py,Vy,Ay,Bc,Bv,Br}}]]
        const T vx = x[1];
        const T vy = x[4];
        const T speed_2 = vx * vx + vy * vy;
        const T speed = std::sqrt(speed_2);
        return {
                {0,    vx / speed, 0, 0,   vy / speed, 0, 0, 0, 0},
                {0, -vy / speed_2, 0, 0, vx / speed_2, 0, 1, 0, 1}
        };
}

template <typename T>
numerical::Vector<2, T> speed_direction_residual(const numerical::Vector<2, T>& a, const numerical::Vector<2, T>& b)
{
        numerical::Vector<2, T> res = a - b;
        res[1] = normalize_angle(res[1]);
        return res;
}

//

template <typename T>
numerical::Matrix<3, 3, T> direction_acceleration_r(
        const numerical::Vector<1, T>& direction_variance,
        const numerical::Vector<2, T>& acceleration_variance)
{
        const numerical::Vector<1, T>& dv = direction_variance;
        const numerical::Vector<2, T>& av = acceleration_variance;
        return numerical::make_diagonal_matrix<3, T>({dv[0], av[0], av[1]});
}

template <typename T>
numerical::Vector<3, T> direction_acceleration_h(const numerical::Vector<9, T>& x)
{
        // angle = atan(vy, vx) + angle + angle_r
        // ax = ax*cos(angle) - ay*sin(angle)
        // ay = ax*sin(angle) + ay*cos(angle)
        const T vx = x[1];
        const T ax = x[2];
        const T vy = x[4];
        const T ay = x[5];
        const T angle = x[6];
        const T angle_r = x[8];
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        return {
                std::atan2(vy, vx) + angle + angle_r, // angle
                ax * cos - ay * sin, // ax
                ax * sin + ay * cos // ay
        };
}

template <typename T>
numerical::Matrix<3, 9, T> direction_acceleration_hj(const numerical::Vector<9, T>& x)
{
        // angle = atan(vy, vx) + angle + angle_r
        // ax = ax*cos(angle) - ay*sin(angle)
        // ay = ax*sin(angle) + ay*cos(angle)
        // Jacobian
        // mAngle=ArcTan[Vx,Vy]+Bc+Br;
        // mAx=(Ax*Cos[Bc]-Ay*Sin[Bc]);
        // mAy=(Ax*Sin[Bc]+Ay*Cos[Bc]);
        // Simplify[D[{mAngle,mAx,mAy},{{Px,Vx,Ax,Py,Vy,Ay,Bc,Bv,Br}}]]
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
                {0, -vy / s_2,   0, 0, vx / s_2,    0,   1, 0, 1},
                {0,         0, cos, 0,        0, -sin, a_1, 0, 0},
                {0,         0, sin, 0,        0,  cos, a_2, 0, 0}
        };
}

template <typename T>
numerical::Vector<3, T> direction_acceleration_residual(
        const numerical::Vector<3, T>& a,
        const numerical::Vector<3, T>& b)
{
        numerical::Vector<3, T> res = a - b;
        res[0] = normalize_angle(res[0]);
        return res;
}

//

template <typename T>
numerical::Matrix<2, 2, T> acceleration_r(const numerical::Vector<2, T>& acceleration_variance)
{
        return numerical::make_diagonal_matrix(acceleration_variance);
}

template <typename T>
numerical::Vector<2, T> acceleration_h(const numerical::Vector<9, T>& x)
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
numerical::Matrix<2, 9, T> acceleration_hj(const numerical::Vector<9, T>& x)
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

template <typename T>
numerical::Vector<2, T> acceleration_residual(const numerical::Vector<2, T>& a, const numerical::Vector<2, T>& b)
{
        return a - b;
}

//

template <typename T>
numerical::Matrix<1, 1, T> direction_r(const numerical::Vector<1, T>& direction_variance)
{
        const numerical::Vector<1, T>& dv = direction_variance;
        return {{dv[0]}};
}

template <typename T>
numerical::Vector<1, T> direction_h(const numerical::Vector<9, T>& x)
{
        // angle = atan(vy, vx) + angle + angle_r
        const T vx = x[1];
        const T vy = x[4];
        const T angle = x[6];
        const T angle_r = x[8];
        return numerical::Vector<1, T>{
                std::atan2(vy, vx) + angle + angle_r // angle
        };
}

template <typename T>
numerical::Matrix<1, 9, T> direction_hj(const numerical::Vector<9, T>& x)
{
        // angle = atan(vy, vx) + angle + angle_r
        // Jacobian
        // mAngle=ArcTan[Vx,Vy]+Bc+Br;
        // Simplify[D[{mAngle},{{Px,Vx,Ax,Py,Vy,Ay,Bc,Bv,Br}}]]
        const T vx = x[1];
        const T vy = x[4];
        const T s_2 = vx * vx + vy * vy;
        return {
                {0, -vy / s_2, 0, 0, vx / s_2, 0, 1, 0, 1}
        };
}

template <typename T>
numerical::Vector<1, T> direction_residual(const numerical::Vector<1, T>& a, const numerical::Vector<1, T>& b)
{
        numerical::Vector<1, T> res = a - b;
        res[0] = normalize_angle(res[0]);
        return res;
}

//

template <typename T>
numerical::Matrix<1, 1, T> speed_r(const numerical::Vector<1, T>& speed_variance)
{
        const numerical::Vector<1, T>& sv = speed_variance;
        return {{sv[0]}};
}

template <typename T>
numerical::Vector<1, T> speed_h(const numerical::Vector<9, T>& x)
{
        // speed = sqrt(vx*vx + vy*vy)
        const T vx = x[1];
        const T vy = x[4];
        return numerical::Vector<1, T>{
                std::sqrt(vx * vx + vy * vy) // speed
        };
}

template <typename T>
numerical::Matrix<1, 9, T> speed_hj(const numerical::Vector<9, T>& x)
{
        // speed = sqrt(vx*vx + vy*vy)
        // Jacobian
        // mSpeed=Sqrt[Vx*Vx+Vy*Vy];
        // Simplify[D[{mSpeed},{{Px,Vx,Ax,Py,Vy,Ay,Bc,Bv,Br}}]]
        const T vx = x[1];
        const T vy = x[4];
        const T speed = std::sqrt(vx * vx + vy * vy);
        return {
                {0, vx / speed, 0, 0, vy / speed, 0, 0, 0, 0}
        };
}

template <typename T>
numerical::Vector<1, T> speed_residual(const numerical::Vector<1, T>& a, const numerical::Vector<1, T>& b)
{
        return a - b;
}

//

template <typename T>
numerical::Matrix<3, 3, T> speed_acceleration_r(
        const numerical::Vector<1, T>& speed_variance,
        const numerical::Vector<2, T>& acceleration_variance)
{
        const numerical::Vector<1, T>& sv = speed_variance;
        const numerical::Vector<2, T>& av = acceleration_variance;
        return numerical::make_diagonal_matrix<3, T>({sv[0], av[0], av[1]});
}

template <typename T>
numerical::Vector<3, T> speed_acceleration_h(const numerical::Vector<9, T>& x)
{
        // speed = sqrt(vx*vx + vy*vy)
        // ax = ax*cos(angle) - ay*sin(angle)
        // ay = ax*sin(angle) + ay*cos(angle)
        const T vx = x[1];
        const T ax = x[2];
        const T vy = x[4];
        const T ay = x[5];
        const T angle = x[6];
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        return {
                std::sqrt(vx * vx + vy * vy), // speed
                ax * cos - ay * sin, // ax
                ax * sin + ay * cos // ay
        };
}

template <typename T>
numerical::Matrix<3, 9, T> speed_acceleration_hj(const numerical::Vector<9, T>& x)
{
        // speed = sqrt(vx*vx + vy*vy)
        // ax = ax*cos(angle) - ay*sin(angle)
        // ay = ax*sin(angle) + ay*cos(angle)
        // Jacobian
        // mSpeed=Sqrt[Vx*Vx+Vy*Vy];
        // mAx=(Ax*Cos[Bc]-Ay*Sin[Bc]);
        // mAy=(Ax*Sin[Bc]+Ay*Cos[Bc]);
        // Simplify[D[{mSpeed,mAx,mAy},{{Px,Vx,Ax,Py,Vy,Ay,Bc,Bv,Br}}]]
        const T vx = x[1];
        const T ax = x[2];
        const T vy = x[4];
        const T ay = x[5];
        const T angle = x[6];
        const T speed = std::sqrt(vx * vx + vy * vy);
        const T cos = std::cos(angle);
        const T sin = std::sin(angle);
        const T a_1 = -ax * sin - ay * cos;
        const T a_2 = ax * cos - ay * sin;
        return {
                {0, vx / speed,   0, 0, vy / speed,    0,   0, 0, 0},
                {0,          0, cos, 0,          0, -sin, a_1, 0, 0},
                {0,          0, sin, 0,          0,  cos, a_2, 0, 0}
        };
}

template <typename T>
numerical::Vector<3, T> speed_acceleration_residual(const numerical::Vector<3, T>& a, const numerical::Vector<3, T>& b)
{
        return a - b;
}

//

template <typename T>
class Filter final : public FilterEkf<T>
{
        const T position_variance_;
        const T angle_variance_;
        const T angle_r_variance_;
        std::optional<core::Ekf<9, T>> filter_;

        [[nodiscard]] numerical::Vector<2, T> velocity() const
        {
                ASSERT(filter_);

                return {filter_->x()[1], filter_->x()[4]};
        }

        [[nodiscard]] numerical::Matrix<2, 2, T> velocity_p() const
        {
                ASSERT(filter_);

                return {
                        {filter_->p()[1, 1], filter_->p()[1, 4]},
                        {filter_->p()[4, 1], filter_->p()[4, 4]}
                };
        }

        void reset(
                const numerical::Vector<4, T>& position_velocity,
                const numerical::Matrix<4, 4, T>& position_velocity_p,
                const Init<T>& init) override
        {
                filter_.emplace(x(position_velocity, init), p(position_velocity_p, init));
        }

        void predict(const T dt) override
        {
                ASSERT(filter_);
                ASSERT(com::check_dt(dt));

                const numerical::Matrix<9, 9, T> f_matrix = f(dt);
                filter_->predict(
                        [&](const numerical::Vector<9, T>& x)
                        {
                                return f_matrix * x;
                        },
                        [&](const numerical::Vector<9, T>& /*x*/)
                        {
                                return f_matrix;
                        },
                        q(dt, position_variance_, angle_variance_, angle_r_variance_));
        }

        void update_position(const Measurement<2, T>& position, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_h<T>, position_hj<T>, position_r(position.variance), position.value, add_x<T>,
                        position_residual<T>, THETA<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_position_speed(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_speed_h<T>, position_speed_hj<T>, position_speed_r(position.variance, speed.variance),
                        numerical::Vector<3, T>(position.value[0], position.value[1], speed.value[0]), add_x<T>,
                        position_speed_residual<T>, THETA<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_position_speed_direction_acceleration(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_speed_direction_acceleration_h<T>, position_speed_direction_acceleration_hj<T>,
                        position_speed_direction_acceleration_r(
                                position.variance, speed.variance, direction.variance, acceleration.variance),
                        numerical::Vector<6, T>(
                                position.value[0], position.value[1], speed.value[0], direction.value[0],
                                acceleration.value[0], acceleration.value[1]),
                        add_x<T>, position_speed_direction_acceleration_residual<T>, THETA<T>, gate,
                        NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_position_speed_direction(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_speed_direction_h<T>, position_speed_direction_hj<T>,
                        position_speed_direction_r(position.variance, speed.variance, direction.variance),
                        numerical::Vector<4, T>(
                                position.value[0], position.value[1], speed.value[0], direction.value[0]),
                        add_x<T>, position_speed_direction_residual<T>, THETA<T>, gate, NORMALIZED_INNOVATION,
                        LIKELIHOOD);
        }

        void update_position_speed_acceleration(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_speed_acceleration_h<T>, position_speed_acceleration_hj<T>,
                        position_speed_acceleration_r(position.variance, speed.variance, acceleration.variance),
                        numerical::Vector<5, T>(
                                position.value[0], position.value[1], speed.value[0], acceleration.value[0],
                                acceleration.value[1]),
                        add_x<T>, position_speed_acceleration_residual<T>, THETA<T>, gate, NORMALIZED_INNOVATION,
                        LIKELIHOOD);
        }

        void update_position_direction_acceleration(
                const Measurement<2, T>& position,
                const Measurement<1, T>& direction,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_direction_acceleration_h<T>, position_direction_acceleration_hj<T>,
                        position_direction_acceleration_r(position.variance, direction.variance, acceleration.variance),
                        numerical::Vector<5, T>(
                                position.value[0], position.value[1], direction.value[0], acceleration.value[0],
                                acceleration.value[1]),
                        add_x<T>, position_direction_acceleration_residual<T>, THETA<T>, gate, NORMALIZED_INNOVATION,
                        LIKELIHOOD);
        }

        void update_position_direction(
                const Measurement<2, T>& position,
                const Measurement<1, T>& direction,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_direction_h<T>, position_direction_hj<T>,
                        position_direction_r(position.variance, direction.variance),
                        numerical::Vector<3, T>(position.value[0], position.value[1], direction.value[0]), add_x<T>,
                        position_direction_residual<T>, THETA<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_position_acceleration(
                const Measurement<2, T>& position,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_acceleration_h<T>, position_acceleration_hj<T>,
                        position_acceleration_r(position.variance, acceleration.variance),
                        numerical::Vector<4, T>(
                                position.value[0], position.value[1], acceleration.value[0], acceleration.value[1]),
                        add_x<T>, position_acceleration_residual<T>, THETA<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_speed_direction_acceleration(
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        speed_direction_acceleration_h<T>, speed_direction_acceleration_hj<T>,
                        speed_direction_acceleration_r(speed.variance, direction.variance, acceleration.variance),
                        numerical::Vector<4, T>(
                                speed.value[0], direction.value[0], acceleration.value[0], acceleration.value[1]),
                        add_x<T>, speed_direction_acceleration_residual<T>, THETA<T>, gate, NORMALIZED_INNOVATION,
                        LIKELIHOOD);
        }

        void update_speed_direction(
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        speed_direction_h<T>, speed_direction_hj<T>,
                        speed_direction_r(speed.variance, direction.variance),
                        numerical::Vector<2, T>(speed.value[0], direction.value[0]), add_x<T>,
                        speed_direction_residual<T>, THETA<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_direction_acceleration(
                const Measurement<1, T>& direction,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        direction_acceleration_h<T>, direction_acceleration_hj<T>,
                        direction_acceleration_r(direction.variance, acceleration.variance),
                        numerical::Vector<3, T>(direction.value[0], acceleration.value[0], acceleration.value[1]),
                        add_x<T>, direction_acceleration_residual<T>, THETA<T>, gate, NORMALIZED_INNOVATION,
                        LIKELIHOOD);
        }

        void update_acceleration(const Measurement<2, T>& acceleration, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        acceleration_h<T>, acceleration_hj<T>, acceleration_r(acceleration.variance),
                        acceleration.value, add_x<T>, acceleration_residual<T>, THETA<T>, gate, NORMALIZED_INNOVATION,
                        LIKELIHOOD);
        }

        void update_direction(const Measurement<1, T>& direction, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        direction_h<T>, direction_hj<T>, direction_r(direction.variance),
                        numerical::Vector<1, T>(direction.value), add_x<T>, direction_residual<T>, THETA<T>, gate,
                        NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_speed(const Measurement<1, T>& speed, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        speed_h<T>, speed_hj<T>, speed_r(speed.variance), numerical::Vector<1, T>(speed.value),
                        add_x<T>, speed_residual<T>, THETA<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_speed_acceleration(
                const Measurement<1, T>& speed,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        speed_acceleration_h<T>, speed_acceleration_hj<T>,
                        speed_acceleration_r(speed.variance, acceleration.variance),
                        numerical::Vector<3, T>(speed.value[0], acceleration.value[0], acceleration.value[1]), add_x<T>,
                        speed_acceleration_residual<T>, THETA<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        [[nodiscard]] numerical::Vector<2, T> position() const override
        {
                ASSERT(filter_);

                return {filter_->x()[0], filter_->x()[3]};
        }

        [[nodiscard]] numerical::Matrix<2, 2, T> position_p() const override
        {
                ASSERT(filter_);

                return {
                        {filter_->p()[0, 0], filter_->p()[0, 3]},
                        {filter_->p()[3, 0], filter_->p()[3, 3]}
                };
        }

        [[nodiscard]] T speed() const override
        {
                return velocity().norm();
        }

        [[nodiscard]] T speed_p() const override
        {
                return com::compute_speed_p(velocity(), velocity_p());
        }

        [[nodiscard]] T angle() const override
        {
                ASSERT(filter_);

                return filter_->x()[6];
        }

        [[nodiscard]] T angle_p() const override
        {
                ASSERT(filter_);

                return filter_->p()[6, 6];
        }

        [[nodiscard]] T angle_speed() const override
        {
                ASSERT(filter_);

                return filter_->x()[7];
        }

        [[nodiscard]] T angle_speed_p() const override
        {
                ASSERT(filter_);

                return filter_->p()[7, 7];
        }

        [[nodiscard]] T angle_r() const override
        {
                ASSERT(filter_);

                return filter_->x()[8];
        }

        [[nodiscard]] T angle_r_p() const override
        {
                ASSERT(filter_);

                return filter_->p()[8, 8];
        }

public:
        Filter(const T position_variance, const T angle_variance, const T angle_r_variance)
                : position_variance_(position_variance),
                  angle_variance_(angle_variance),
                  angle_r_variance_(angle_r_variance)
        {
        }
};
}

template <typename T>
std::unique_ptr<FilterEkf<T>> create_filter_ekf(
        const T position_variance,
        const T angle_variance,
        const T angle_r_variance)
{
        return std::make_unique<Filter<T>>(position_variance, angle_variance, angle_r_variance);
}

#define TEMPLATE(T) template std::unique_ptr<FilterEkf<T>> create_filter_ekf(T, T, T);

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)
}
