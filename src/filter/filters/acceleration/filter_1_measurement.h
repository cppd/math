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

#include <src/filter/filters/com/angle.h>
#include <src/filter/filters/com/speed.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

namespace ns::filter::filters::acceleration::filter_1_measurement
{
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
                com::speed(vx, vy) // speed
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
numerical::Vector<6, T> position_speed_direction_acceleration_h(
        const numerical::Vector<9, T>& x,
        const T reference_angle)
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
        const auto [ax_r, ay_r] = com::rotate({ax, ay}, angle);
        return {
                px, // px
                py, // py
                com::speed(vx, vy), // speed
                com::unwrap_angle(reference_angle, com::angle(vx, vy)) + angle + angle_r, // angle
                ax_r, // ax
                ay_r // ay
        };
}

template <typename T>
numerical::Vector<6, T> position_speed_direction_acceleration_residual(
        const numerical::Vector<6, T>& a,
        const numerical::Vector<6, T>& b)
{
        numerical::Vector<6, T> res = a - b;
        res[3] = com::wrap_angle(res[3]);
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
numerical::Vector<4, T> position_speed_direction_h(const numerical::Vector<9, T>& x, const T reference_angle)
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
                com::speed(vx, vy), // speed
                com::unwrap_angle(reference_angle, com::angle(vx, vy)) + angle + angle_r // angle
        };
}

template <typename T>
numerical::Vector<4, T> position_speed_direction_residual(
        const numerical::Vector<4, T>& a,
        const numerical::Vector<4, T>& b)
{
        numerical::Vector<4, T> res = a - b;
        res[3] = com::wrap_angle(res[3]);
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
        const auto [ax_r, ay_r] = com::rotate({ax, ay}, angle);
        return {
                px, // px
                py, // py
                com::speed(vx, vy), // speed
                ax_r, // ax
                ay_r // ay
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
numerical::Vector<5, T> position_direction_acceleration_h(const numerical::Vector<9, T>& x, const T reference_angle)
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
        const auto [ax_r, ay_r] = com::rotate({ax, ay}, angle);
        return {
                px, // px
                py, // py
                com::unwrap_angle(reference_angle, com::angle(vx, vy)) + angle + angle_r, // angle
                ax_r, // ax
                ay_r // ay
        };
}

template <typename T>
numerical::Vector<5, T> position_direction_acceleration_residual(
        const numerical::Vector<5, T>& a,
        const numerical::Vector<5, T>& b)
{
        numerical::Vector<5, T> res = a - b;
        res[2] = com::wrap_angle(res[2]);
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
numerical::Vector<3, T> position_direction_h(const numerical::Vector<9, T>& x, const T reference_angle)
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
                com::unwrap_angle(reference_angle, com::angle(vx, vy)) + angle + angle_r // angle
        };
}

template <typename T>
numerical::Vector<3, T> position_direction_residual(const numerical::Vector<3, T>& a, const numerical::Vector<3, T>& b)
{
        numerical::Vector<3, T> res = a - b;
        res[2] = com::wrap_angle(res[2]);
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
        const auto [ax_r, ay_r] = com::rotate({ax, ay}, angle);
        return {
                px, // px
                py, // py
                ax_r, // ax
                ay_r // ay
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
numerical::Vector<4, T> speed_direction_acceleration_h(const numerical::Vector<9, T>& x, const T reference_angle)
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
        const auto [ax_r, ay_r] = com::rotate({ax, ay}, angle);
        return {
                com::speed(vx, vy), // speed
                com::unwrap_angle(reference_angle, com::angle(vx, vy)) + angle + angle_r, // angle
                ax_r, // ax
                ay_r // ay
        };
}

template <typename T>
numerical::Vector<4, T> speed_direction_acceleration_residual(
        const numerical::Vector<4, T>& a,
        const numerical::Vector<4, T>& b)
{
        numerical::Vector<4, T> res = a - b;
        res[1] = com::wrap_angle(res[1]);
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
numerical::Vector<2, T> speed_direction_h(const numerical::Vector<9, T>& x, const T reference_angle)
{
        // speed = sqrt(vx*vx + vy*vy)
        // angle = atan(vy, vx) + angle + angle_r
        const T vx = x[1];
        const T vy = x[4];
        const T angle = x[6];
        const T angle_r = x[8];
        return {
                com::speed(vx, vy), // speed
                com::unwrap_angle(reference_angle, com::angle(vx, vy)) + angle + angle_r // angle
        };
}

template <typename T>
numerical::Vector<2, T> speed_direction_residual(const numerical::Vector<2, T>& a, const numerical::Vector<2, T>& b)
{
        numerical::Vector<2, T> res = a - b;
        res[1] = com::wrap_angle(res[1]);
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
numerical::Vector<3, T> direction_acceleration_h(const numerical::Vector<9, T>& x, const T reference_angle)
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
        const auto [ax_r, ay_r] = com::rotate({ax, ay}, angle);
        return {
                com::unwrap_angle(reference_angle, com::angle(vx, vy)) + angle + angle_r, // angle
                ax_r, // ax
                ay_r // ay
        };
}

template <typename T>
numerical::Vector<3, T> direction_acceleration_residual(
        const numerical::Vector<3, T>& a,
        const numerical::Vector<3, T>& b)
{
        numerical::Vector<3, T> res = a - b;
        res[0] = com::wrap_angle(res[0]);
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
        const auto [ax_r, ay_r] = com::rotate({ax, ay}, angle);
        return {
                ax_r, // ax
                ay_r // ay
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
numerical::Vector<1, T> direction_h(const numerical::Vector<9, T>& x, const T reference_angle)
{
        // angle = atan(vy, vx) + angle + angle_r
        const T vx = x[1];
        const T vy = x[4];
        const T angle = x[6];
        const T angle_r = x[8];
        return numerical::Vector<1, T>{
                com::unwrap_angle(reference_angle, com::angle(vx, vy)) + angle + angle_r // angle
        };
}

template <typename T>
numerical::Vector<1, T> direction_residual(const numerical::Vector<1, T>& a, const numerical::Vector<1, T>& b)
{
        numerical::Vector<1, T> res = a - b;
        res[0] = com::wrap_angle(res[0]);
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
                com::speed(vx, vy) // speed
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
        const auto [ax_r, ay_r] = com::rotate({ax, ay}, angle);
        return {
                com::speed(vx, vy), // speed
                ax_r, // ax
                ay_r // ay
        };
}

template <typename T>
numerical::Vector<3, T> speed_acceleration_residual(const numerical::Vector<3, T>& a, const numerical::Vector<3, T>& b)
{
        return a - b;
}
}
