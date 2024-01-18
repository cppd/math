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

#include "filter_1.h"

#include "init.h"

#include <src/com/angle.h>
#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/filters/utility/utility.h>
#include <src/filter/settings/instantiation.h>
#include <src/filter/sigma_points.h>
#include <src/filter/ukf.h>
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
Vector<9, T> x(const Vector<6, T>& position_velocity_acceleration, const Init<T>& init)
{
        ASSERT(is_finite(position_velocity_acceleration));

        Vector<9, T> res;

        for (std::size_t i = 0; i < 6; ++i)
        {
                res[i] = position_velocity_acceleration[i];
        }

        res[6] = init.angle;
        res[7] = init.angle_speed;
        res[8] = init.angle_r;

        return res;
}

template <typename T>
Matrix<9, 9, T> p(const Matrix<6, 6, T>& position_velocity_acceleration_p, const Init<T>& init)
{
        ASSERT(is_finite(position_velocity_acceleration_p));

        Matrix<9, 9, T> res(0);

        for (std::size_t r = 0; r < 6; ++r)
        {
                for (std::size_t c = 0; c < 6; ++c)
                {
                        res[r, c] = position_velocity_acceleration_p[r, c];
                }
        }

        res[6, 6] = init.angle_variance;
        res[7, 7] = init.angle_speed_variance;
        res[8, 8] = init.angle_r_variance;

        return res;
}

template <typename T>
Vector<9, T> x(const Vector<4, T>& position_velocity, const Init<T>& init)
{
        ASSERT(is_finite(position_velocity));

        Vector<9, T> res;

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
Matrix<9, 9, T> p(const Matrix<4, 4, T>& position_velocity_p, const Init<T>& init)
{
        ASSERT(is_finite(position_velocity_p));

        const Matrix<4, 4, T>& p = position_velocity_p;
        static constexpr std::size_t N = 2;

        Matrix<9, 9, T> res(0);

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
[[nodiscard]] Vector<9, T> add_x(const Vector<9, T>& a, const Vector<9, T>& b)
{
        Vector<9, T> res = a + b;
        res[6] = normalize_angle(res[6]);
        res[8] = normalize_angle(res[8]);
        return res;
}

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
        const T dt_2 = power<2>(dt) / 2;
        const T dt_3 = power<3>(dt) / 6;
        const Matrix<9, 4, T> noise_transition{
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
        const Matrix<4, 4, T> covariance{
                {p, 0, 0,   0},
                {0, p, 0,   0},
                {0, 0, a,   0},
                {0, 0, 0, a_r}
        };

        return noise_transition * covariance * noise_transition.transposed();
}

//

template <typename T>
Matrix<2, 2, T> position_r(const Vector<2, T>& position_variance)
{
        return make_diagonal_matrix(position_variance);
}

template <typename T>
Vector<2, T> position_h(const Vector<9, T>& x)
{
        // px = px
        // py = py
        return {x[0], x[3]};
}

template <typename T>
Vector<2, T> position_residual(const Vector<2, T>& a, const Vector<2, T>& b)
{
        return a - b;
}

//

template <typename T>
Matrix<3, 3, T> position_speed_r(const Vector<2, T>& position_variance, const Vector<1, T>& speed_variance)
{
        const Vector<2, T>& pv = position_variance;
        const Vector<1, T>& sv = speed_variance;
        return make_diagonal_matrix<3, T>({pv[0], pv[1], sv[0]});
}

template <typename T>
Vector<3, T> position_speed_h(const Vector<9, T>& x)
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
                std::sqrt(vx * vx + vy * vy) // speed
        };
}

template <typename T>
Vector<3, T> position_speed_residual(const Vector<3, T>& a, const Vector<3, T>& b)
{
        return a - b;
}

//

template <typename T>
Matrix<6, 6, T> position_speed_direction_acceleration_r(
        const Vector<2, T>& position_variance,
        const Vector<1, T>& speed_variance,
        const Vector<1, T>& direction_variance,
        const Vector<2, T>& acceleration_variance)
{
        const Vector<2, T>& pv = position_variance;
        const Vector<1, T>& sv = speed_variance;
        const Vector<1, T>& dv = direction_variance;
        const Vector<2, T>& av = acceleration_variance;
        return make_diagonal_matrix<6, T>({pv[0], pv[1], sv[0], dv[0], av[0], av[1]});
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
Vector<6, T> position_speed_direction_acceleration_residual(const Vector<6, T>& a, const Vector<6, T>& b)
{
        Vector<6, T> res = a - b;
        res[3] = normalize_angle(res[3]);
        return res;
}

//

template <typename T>
Matrix<4, 4, T> position_speed_direction_r(
        const Vector<2, T>& position_variance,
        const Vector<1, T>& speed_variance,
        const Vector<1, T>& direction_variance)
{
        const Vector<2, T>& pv = position_variance;
        const Vector<1, T>& sv = speed_variance;
        const Vector<1, T>& dv = direction_variance;
        return make_diagonal_matrix<4, T>({pv[0], pv[1], sv[0], dv[0]});
}

template <typename T>
Vector<4, T> position_speed_direction_h(const Vector<9, T>& x)
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
Vector<4, T> position_speed_direction_residual(const Vector<4, T>& a, const Vector<4, T>& b)
{
        Vector<4, T> res = a - b;
        res[3] = normalize_angle(res[3]);
        return res;
}

//

template <typename T>
Matrix<5, 5, T> position_speed_acceleration_r(
        const Vector<2, T>& position_variance,
        const Vector<1, T>& speed_variance,
        const Vector<2, T>& acceleration_variance)
{
        const Vector<2, T>& pv = position_variance;
        const Vector<1, T>& sv = speed_variance;
        const Vector<2, T>& av = acceleration_variance;
        return make_diagonal_matrix<5, T>({pv[0], pv[1], sv[0], av[0], av[1]});
}

template <typename T>
Vector<5, T> position_speed_acceleration_h(const Vector<9, T>& x)
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
Vector<5, T> position_speed_acceleration_residual(const Vector<5, T>& a, const Vector<5, T>& b)
{
        return a - b;
}

//

template <typename T>
Matrix<5, 5, T> position_direction_acceleration_r(
        const Vector<2, T>& position_variance,
        const Vector<1, T>& direction_variance,
        const Vector<2, T>& acceleration_variance)
{
        const Vector<2, T>& pv = position_variance;
        const Vector<1, T>& dv = direction_variance;
        const Vector<2, T>& av = acceleration_variance;
        return make_diagonal_matrix<5, T>({pv[0], pv[1], dv[0], av[0], av[1]});
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
Vector<5, T> position_direction_acceleration_residual(const Vector<5, T>& a, const Vector<5, T>& b)
{
        Vector<5, T> res = a - b;
        res[2] = normalize_angle(res[2]);
        return res;
}

//

template <typename T>
Matrix<3, 3, T> position_direction_r(const Vector<2, T>& position_variance, const Vector<1, T>& direction_variance)
{
        const Vector<2, T>& pv = position_variance;
        const Vector<1, T>& dv = direction_variance;
        return make_diagonal_matrix<3, T>({pv[0], pv[1], dv[0]});
}

template <typename T>
Vector<3, T> position_direction_h(const Vector<9, T>& x)
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
Vector<3, T> position_direction_residual(const Vector<3, T>& a, const Vector<3, T>& b)
{
        Vector<3, T> res = a - b;
        res[2] = normalize_angle(res[2]);
        return res;
}

//

template <typename T>
Matrix<4, 4, T> position_acceleration_r(
        const Vector<2, T>& position_variance,
        const Vector<2, T>& acceleration_variance)
{
        const Vector<2, T>& pv = position_variance;
        const Vector<2, T>& av = acceleration_variance;
        return make_diagonal_matrix<4, T>({pv[0], pv[1], av[0], av[1]});
}

template <typename T>
Vector<4, T> position_acceleration_h(const Vector<9, T>& x)
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
Vector<4, T> position_acceleration_residual(const Vector<4, T>& a, const Vector<4, T>& b)
{
        return a - b;
}

//

template <typename T>
Matrix<4, 4, T> speed_direction_acceleration_r(
        const Vector<1, T>& speed_variance,
        const Vector<1, T>& direction_variance,
        const Vector<2, T>& acceleration_variance)
{
        const Vector<1, T>& sv = speed_variance;
        const Vector<1, T>& dv = direction_variance;
        const Vector<2, T>& av = acceleration_variance;
        return make_diagonal_matrix<4, T>({sv[0], dv[0], av[0], av[1]});
}

template <typename T>
Vector<4, T> speed_direction_acceleration_h(const Vector<9, T>& x)
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
Vector<4, T> speed_direction_acceleration_residual(const Vector<4, T>& a, const Vector<4, T>& b)
{
        Vector<4, T> res = a - b;
        res[1] = normalize_angle(res[1]);
        return res;
}

//

template <typename T>
Matrix<2, 2, T> speed_direction_r(const Vector<1, T>& speed_variance, const Vector<1, T>& direction_variance)
{
        const Vector<1, T>& sv = speed_variance;
        const Vector<1, T>& dv = direction_variance;
        return make_diagonal_matrix<2, T>({sv[0], dv[0]});
}

template <typename T>
Vector<2, T> speed_direction_h(const Vector<9, T>& x)
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
Vector<2, T> speed_direction_residual(const Vector<2, T>& a, const Vector<2, T>& b)
{
        Vector<2, T> res = a - b;
        res[1] = normalize_angle(res[1]);
        return res;
}

//

template <typename T>
Matrix<3, 3, T> direction_acceleration_r(
        const Vector<1, T>& direction_variance,
        const Vector<2, T>& acceleration_variance)
{
        const Vector<1, T>& dv = direction_variance;
        const Vector<2, T>& av = acceleration_variance;
        return make_diagonal_matrix<3, T>({dv[0], av[0], av[1]});
}

template <typename T>
Vector<3, T> direction_acceleration_h(const Vector<9, T>& x)
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
Vector<3, T> direction_acceleration_residual(const Vector<3, T>& a, const Vector<3, T>& b)
{
        Vector<3, T> res = a - b;
        res[0] = normalize_angle(res[0]);
        return res;
}

//

template <typename T>
Matrix<2, 2, T> acceleration_r(const Vector<2, T>& acceleration_variance)
{
        return make_diagonal_matrix(acceleration_variance);
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

template <typename T>
Vector<2, T> acceleration_residual(const Vector<2, T>& a, const Vector<2, T>& b)
{
        return a - b;
}

//

template <typename T>
Matrix<1, 1, T> direction_r(const Vector<1, T>& direction_variance)
{
        const Vector<1, T>& dv = direction_variance;
        return {{dv[0]}};
}

template <typename T>
Vector<1, T> direction_h(const Vector<9, T>& x)
{
        // angle = atan(vy, vx) + angle + angle_r
        const T vx = x[1];
        const T vy = x[4];
        const T angle = x[6];
        const T angle_r = x[8];
        return Vector<1, T>{
                std::atan2(vy, vx) + angle + angle_r // angle
        };
}

template <typename T>
Vector<1, T> direction_residual(const Vector<1, T>& a, const Vector<1, T>& b)
{
        Vector<1, T> res = a - b;
        res[0] = normalize_angle(res[0]);
        return res;
}

//

template <typename T>
Matrix<1, 1, T> speed_r(const Vector<1, T>& speed_variance)
{
        const Vector<1, T>& sv = speed_variance;
        return {{sv[0]}};
}

template <typename T>
Vector<1, T> speed_h(const Vector<9, T>& x)
{
        // speed = sqrt(vx*vx + vy*vy)
        const T vx = x[1];
        const T vy = x[4];
        return Vector<1, T>{
                std::sqrt(vx * vx + vy * vy) // speed
        };
}

template <typename T>
Vector<1, T> speed_residual(const Vector<1, T>& a, const Vector<1, T>& b)
{
        return a - b;
}

//

template <typename T>
Matrix<3, 3, T> speed_acceleration_r(const Vector<1, T>& speed_variance, const Vector<2, T>& acceleration_variance)
{
        const Vector<1, T>& sv = speed_variance;
        const Vector<2, T>& av = acceleration_variance;
        return make_diagonal_matrix<3, T>({sv[0], av[0], av[1]});
}

template <typename T>
Vector<3, T> speed_acceleration_h(const Vector<9, T>& x)
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
Vector<3, T> speed_acceleration_residual(const Vector<3, T>& a, const Vector<3, T>& b)
{
        return a - b;
}

//

template <typename T>
class Filter final : public Filter1<T>
{
        static constexpr bool NORMALIZED_INNOVATION{false};
        static constexpr bool LIKELIHOOD{false};

        const T sigma_points_alpha_;
        const T position_variance_;
        const T angle_variance_;
        const T angle_r_variance_;
        std::optional<Ukf<9, T, SigmaPoints<9, T>>> filter_;

        [[nodiscard]] Vector<2, T> velocity() const
        {
                ASSERT(filter_);

                return {filter_->x()[1], filter_->x()[4]};
        }

        [[nodiscard]] Matrix<2, 2, T> velocity_p() const
        {
                ASSERT(filter_);

                return {
                        {filter_->p()[1, 1], filter_->p()[1, 4]},
                        {filter_->p()[4, 1], filter_->p()[4, 4]}
                };
        }

        void reset(
                const Vector<6, T>& position_velocity_acceleration,
                const Matrix<6, 6, T>& position_velocity_acceleration_p,
                const Init<T>& init) override
        {
                filter_.emplace(
                        create_sigma_points<9, T>(sigma_points_alpha_), x(position_velocity_acceleration, init),
                        p(position_velocity_acceleration_p, init));
        }

        void reset(
                const Vector<4, T>& position_velocity_acceleration,
                const Matrix<4, 4, T>& position_velocity_acceleration_p,
                const Init<T>& init) override
        {
                filter_.emplace(
                        create_sigma_points<9, T>(sigma_points_alpha_), x(position_velocity_acceleration, init),
                        p(position_velocity_acceleration_p, init));
        }

        void predict(const T dt) override
        {
                ASSERT(filter_);
                ASSERT(utility::check_dt(dt));

                filter_->predict(
                        [dt](const Vector<9, T>& x)
                        {
                                return f(dt, x);
                        },
                        q(dt, position_variance_, angle_variance_, angle_r_variance_));
        }

        void update_position(const Measurement<2, T>& position, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_h<T>, position_r(position.variance), position.value, add_x<T>, position_residual<T>,
                        gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_position_speed(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_speed_h<T>, position_speed_r(position.variance, speed.variance),
                        Vector<3, T>(position.value[0], position.value[1], speed.value[0]), add_x<T>,
                        position_speed_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
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
                        position_speed_direction_acceleration_h<T>,
                        position_speed_direction_acceleration_r(
                                position.variance, speed.variance, direction.variance, acceleration.variance),
                        Vector<6, T>(
                                position.value[0], position.value[1], speed.value[0], direction.value[0],
                                acceleration.value[0], acceleration.value[1]),
                        add_x<T>, position_speed_direction_acceleration_residual<T>, gate, NORMALIZED_INNOVATION,
                        LIKELIHOOD);
        }

        void update_position_speed_direction(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_speed_direction_h<T>,
                        position_speed_direction_r(position.variance, speed.variance, direction.variance),
                        Vector<4, T>(position.value[0], position.value[1], speed.value[0], direction.value[0]),
                        add_x<T>, position_speed_direction_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_position_speed_acceleration(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_speed_acceleration_h<T>,
                        position_speed_acceleration_r(position.variance, speed.variance, acceleration.variance),
                        Vector<5, T>(
                                position.value[0], position.value[1], speed.value[0], acceleration.value[0],
                                acceleration.value[1]),
                        add_x<T>, position_speed_acceleration_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_position_direction_acceleration(
                const Measurement<2, T>& position,
                const Measurement<1, T>& direction,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_direction_acceleration_h<T>,
                        position_direction_acceleration_r(position.variance, direction.variance, acceleration.variance),
                        Vector<5, T>(
                                position.value[0], position.value[1], direction.value[0], acceleration.value[0],
                                acceleration.value[1]),
                        add_x<T>, position_direction_acceleration_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_position_direction(
                const Measurement<2, T>& position,
                const Measurement<1, T>& direction,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_direction_h<T>, position_direction_r(position.variance, direction.variance),
                        Vector<3, T>(position.value[0], position.value[1], direction.value[0]), add_x<T>,
                        position_direction_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_position_acceleration(
                const Measurement<2, T>& position,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_acceleration_h<T>, position_acceleration_r(position.variance, acceleration.variance),
                        Vector<4, T>(
                                position.value[0], position.value[1], acceleration.value[0], acceleration.value[1]),
                        add_x<T>, position_acceleration_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_speed_direction_acceleration(
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        speed_direction_acceleration_h<T>,
                        speed_direction_acceleration_r(speed.variance, direction.variance, acceleration.variance),
                        Vector<4, T>(speed.value[0], direction.value[0], acceleration.value[0], acceleration.value[1]),
                        add_x<T>, speed_direction_acceleration_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_speed_direction(
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        speed_direction_h<T>, speed_direction_r(speed.variance, direction.variance),
                        Vector<2, T>(speed.value[0], direction.value[0]), add_x<T>, speed_direction_residual<T>, gate,
                        NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_direction_acceleration(
                const Measurement<1, T>& direction,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        direction_acceleration_h<T>,
                        direction_acceleration_r(direction.variance, acceleration.variance),
                        Vector<3, T>(direction.value[0], acceleration.value[0], acceleration.value[1]), add_x<T>,
                        direction_acceleration_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_acceleration(const Measurement<2, T>& acceleration, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        acceleration_h<T>, acceleration_r(acceleration.variance), acceleration.value, add_x<T>,
                        acceleration_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_direction(const Measurement<1, T>& direction, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        direction_h<T>, direction_r(direction.variance), Vector<1, T>(direction.value), add_x<T>,
                        direction_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_speed(const Measurement<1, T>& speed, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        speed_h<T>, speed_r(speed.variance), Vector<1, T>(speed.value), add_x<T>, speed_residual<T>,
                        gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_speed_acceleration(
                const Measurement<1, T>& speed,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        speed_acceleration_h<T>, speed_acceleration_r(speed.variance, acceleration.variance),
                        Vector<3, T>(speed.value[0], acceleration.value[0], acceleration.value[1]), add_x<T>,
                        speed_acceleration_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        [[nodiscard]] Vector<2, T> position() const override
        {
                ASSERT(filter_);

                return {filter_->x()[0], filter_->x()[3]};
        }

        [[nodiscard]] Matrix<2, 2, T> position_p() const override
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
                return utility::compute_speed_p(velocity(), velocity_p());
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
        Filter(const T sigma_points_alpha, const T position_variance, const T angle_variance, const T angle_r_variance)
                : sigma_points_alpha_(sigma_points_alpha),
                  position_variance_(position_variance),
                  angle_variance_(angle_variance),
                  angle_r_variance_(angle_r_variance)
        {
        }
};
}

template <typename T>
std::unique_ptr<Filter1<T>> create_filter_1(
        const T sigma_points_alpha,
        const T position_variance,
        const T angle_variance,
        const T angle_r_variance)
{
        return std::make_unique<Filter<T>>(sigma_points_alpha, position_variance, angle_variance, angle_r_variance);
}

#define TEMPLATE(T) template std::unique_ptr<Filter1<T>> create_filter_1(T, T, T, T);

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)
}
