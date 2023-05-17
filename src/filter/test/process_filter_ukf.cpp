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

#include "process_filter_ukf.h"

#include "utility.h"

namespace ns::filter::test
{
namespace
{
template <std::size_t N, typename T>
SigmaPoints<N, T> create_sigma_points()
{
        constexpr T ALPHA = 0.1;
        constexpr T BETA = 2; // 2 for Gaussian
        constexpr T KAPPA = 3 - T{N};

        return {ALPHA, BETA, KAPPA};
}

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

struct ResidualX final
{
        template <typename T>
        [[nodiscard]] Vector<9, T> operator()(const Vector<9, T>& a, const Vector<9, T>& b) const
        {
                Vector<9, T> res = a - b;
                res[6] = normalize_angle(res[6]);
                res[8] = normalize_angle(res[8]);
                return res;
        }
};

struct MeanX final
{
        template <typename T, std::size_t COUNT>
        [[nodiscard]] Vector<9, T> operator()(const std::array<Vector<9, T>, COUNT>& p, const Vector<COUNT, T>& w) const
        {
                Vector<9, T> x(0);
                T cos6 = 0;
                T sin6 = 0;
                T cos8 = 0;
                T sin8 = 0;
                for (std::size_t i = 0; i < COUNT; ++i)
                {
                        const Vector<9, T>& pi = p[i];
                        const T wi = w[i];
                        x[0] += wi * pi[0];
                        x[1] += wi * pi[1];
                        x[2] += wi * pi[2];
                        x[3] += wi * pi[3];
                        x[4] += wi * pi[4];
                        x[5] += wi * pi[5];
                        cos6 += wi * std::cos(pi[6]);
                        sin6 += wi * std::sin(pi[6]);
                        x[7] += wi * pi[7];
                        cos8 += wi * std::cos(pi[8]);
                        sin8 += wi * std::sin(pi[8]);
                }
                x[6] = std::atan2(sin6, cos6);
                x[8] = std::atan2(sin8, cos8);
                return x;
        }
};

template <typename T>
Vector<9, T> f(const T dt, const Vector<9, T>& x)
{
        const T dt_2 = square(dt) / 2;

        const T px = x[0];
        const T vx = x[1];
        const T ax = x[2];
        const T py = x[3];
        const T vy = x[4];
        const T ay = x[5];
        const T angle = x[6];
        const T angle_v = x[7];
        const T angle_r = x[8];

        return {
                px + dt * vx + dt_2 * ax, // px
                vx + dt * ax, // vx
                ax, // ax
                py + dt * vy + dt_2 * ay, // py
                vy + dt * ay, // vy
                ay, // ay
                angle + dt * angle_v, // angle
                angle_v, // angle_v
                angle_r // angle_r
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

struct PositionSpeedDirectionAccelerationResidual final
{
        template <typename T>
        Vector<6, T> operator()(const Vector<6, T>& a, const Vector<6, T>& b) const
        {
                Vector<6, T> res = a - b;
                res[3] = normalize_angle(res[3]);
                return res;
        }
};

struct PositionSpeedDirectionAccelerationMean final
{
        template <typename T, std::size_t COUNT>
        Vector<6, T> operator()(const std::array<Vector<6, T>, COUNT>& p, const Vector<COUNT, T>& w) const
        {
                Vector<6, T> x(0);
                T cos3 = 0;
                T sin3 = 0;
                for (std::size_t i = 0; i < COUNT; ++i)
                {
                        const Vector<6, T>& pi = p[i];
                        const T wi = w[i];
                        x[0] += wi * pi[0];
                        x[1] += wi * pi[1];
                        x[2] += wi * pi[2];
                        cos3 += wi * std::cos(pi[3]);
                        sin3 += wi * std::sin(pi[3]);
                        x[4] += wi * pi[4];
                        x[5] += wi * pi[5];
                }
                x[3] = std::atan2(sin3, cos3);
                return x;
        }
};

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

struct PositionDirectionAccelerationResidual final
{
        template <typename T>
        Vector<5, T> operator()(const Vector<5, T>& a, const Vector<5, T>& b) const
        {
                Vector<5, T> res = a - b;
                res[2] = normalize_angle(res[2]);
                return res;
        }
};

struct PositionDirectionAccelerationMean final
{
        template <typename T, std::size_t COUNT>
        Vector<5, T> operator()(const std::array<Vector<5, T>, COUNT>& p, const Vector<COUNT, T>& w) const
        {
                Vector<5, T> x(0);
                T cos2 = 0;
                T sin2 = 0;
                for (std::size_t i = 0; i < COUNT; ++i)
                {
                        const Vector<5, T>& pi = p[i];
                        const T wi = w[i];
                        x[0] += wi * pi[0];
                        x[1] += wi * pi[1];
                        cos2 += wi * std::cos(pi[2]);
                        sin2 += wi * std::sin(pi[2]);
                        x[3] += wi * pi[3];
                        x[4] += wi * pi[4];
                }
                x[2] = std::atan2(sin2, cos2);
                return x;
        }
};

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
        return {
                ax * cos - ay * sin, // ax
                ax * sin + ay * cos // ay
        };
}
}

template <typename T>
ProcessFilterUkf<T>::ProcessFilterUkf(
        const T dt,
        const T position_variance,
        const T angle_variance,
        const T angle_r_variance,
        const Vector<9, T>& x,
        const Matrix<9, 9, T>& p)
        : filter_(create_sigma_points<9, T>(), x, p),
          dt_(dt),
          q_(q(dt, position_variance, angle_variance, angle_r_variance))
{
}

template <typename T>
void ProcessFilterUkf<T>::predict()
{
        filter_.predict(
                [&](const Vector<9, T>& x)
                {
                        return f(dt_, x);
                },
                q_, MeanX(), ResidualX());
}

template <typename T>
void ProcessFilterUkf<T>::update_position(const Vector<2, T>& position, const T position_variance)
{
        filter_.update(position_h<T>, position_r(position_variance), position);
}

template <typename T>
void ProcessFilterUkf<T>::update_position_speed_direction_acceleration(
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
                position_speed_direction_acceleration_h<T>,
                position_speed_direction_acceleration_r(
                        position_variance, speed_variance, direction_variance, acceleration_variance),
                Vector<6, T>(position[0], position[1], speed, direction, acceleration[0], acceleration[1]),
                PositionSpeedDirectionAccelerationMean(), ResidualX(), PositionSpeedDirectionAccelerationResidual(),
                AddX());
}

template <typename T>
void ProcessFilterUkf<T>::update_position_direction_acceleration(
        const Vector<2, T>& position,
        const T direction,
        const Vector<2, T>& acceleration,
        const T position_variance,
        const T direction_variance,
        const T acceleration_variance)
{
        filter_.update(
                position_direction_acceleration_h<T>,
                position_direction_acceleration_r(position_variance, direction_variance, acceleration_variance),
                Vector<5, T>(position[0], position[1], direction, acceleration[0], acceleration[1]),
                PositionDirectionAccelerationMean(), ResidualX(), PositionDirectionAccelerationResidual(), AddX());
}

template <typename T>
void ProcessFilterUkf<T>::update_acceleration(const Vector<2, T>& acceleration, const T acceleration_variance)
{
        filter_.update(acceleration_h<T>, acceleration_r(acceleration_variance), acceleration);
}

template <typename T>
Vector<2, T> ProcessFilterUkf<T>::position() const
{
        return {filter_.x()[0], filter_.x()[3]};
}

template <typename T>
Matrix<2, 2, T> ProcessFilterUkf<T>::position_p() const
{
        return {
                {filter_.p()(0, 0), filter_.p()(0, 3)},
                {filter_.p()(3, 0), filter_.p()(3, 3)}
        };
}

template <typename T>
T ProcessFilterUkf<T>::speed() const
{
        return Vector<2, T>(filter_.x()[1], filter_.x()[4]).norm();
}

template <typename T>
T ProcessFilterUkf<T>::angle() const
{
        return filter_.x()[6];
}

template <typename T>
T ProcessFilterUkf<T>::angle_speed() const
{
        return filter_.x()[7];
}

template <typename T>
T ProcessFilterUkf<T>::angle_p() const
{
        return filter_.p()(6, 6);
}

template <typename T>
T ProcessFilterUkf<T>::angle_r() const
{
        return filter_.x()[8];
}

template <typename T>
T ProcessFilterUkf<T>::angle_r_p() const
{
        return filter_.p()(8, 8);
}

template class ProcessFilterUkf<float>;
template class ProcessFilterUkf<double>;
template class ProcessFilterUkf<long double>;
}
