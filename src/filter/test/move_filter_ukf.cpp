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

#include "move_filter_ukf.h"

#include "utility.h"

#include "../sigma_points.h"
#include "../ukf.h"

#include <src/com/angle.h>
#include <src/com/error.h>
#include <src/com/exponent.h>

#include <optional>

namespace ns::filter::test
{
namespace
{
template <typename T>
constexpr T SIGMA_POINTS_BETA = 2; // 2 for Gaussian
template <std::size_t N, typename T>
constexpr T SIGMA_POINTS_KAPPA = 3 - T{N};

template <typename T>
Vector<8, T> x(const Vector<6, T>& position_velocity_acceleration, const T angle)
{
        ASSERT(is_finite(position_velocity_acceleration));

        Vector<8, T> res;

        for (std::size_t i = 0; i < 6; ++i)
        {
                res[i] = position_velocity_acceleration[i];
        }

        res[6] = angle;
        res[7] = MoveFilterInit<T>::ANGLE_SPEED;

        return res;
}

template <typename T>
Matrix<8, 8, T> p(const Matrix<6, 6, T>& position_velocity_acceleration_p)
{
        ASSERT(is_finite(position_velocity_acceleration_p));

        Matrix<8, 8, T> res(0);

        for (std::size_t r = 0; r < 6; ++r)
        {
                for (std::size_t c = 0; c < 6; ++c)
                {
                        res(r, c) = position_velocity_acceleration_p(r, c);
                }
        }

        res(6, 6) = MoveFilterInit<T>::ANGLE_VARIANCE;
        res(7, 7) = MoveFilterInit<T>::ANGLE_SPEED_VARIANCE;

        return res;
}
struct AddX final
{
        template <typename T>
        [[nodiscard]] Vector<8, T> operator()(const Vector<8, T>& a, const Vector<8, T>& b) const
        {
                Vector<8, T> res = a + b;
                res[6] = normalize_angle(res[6]);
                return res;
        }
};

template <typename T>
Vector<8, T> f(const T dt, const Vector<8, T>& x)
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

        return {
                px + dt * vx + dt_2 * ax, // px
                vx + dt * ax, // vx
                ax, // ax
                py + dt * vy + dt_2 * ay, // py
                vy + dt * ay, // vy
                ay, // ay
                angle + dt * angle_v, // angle
                angle_v // angle_v
        };
}

template <typename T>
constexpr Matrix<8, 8, T> q(const T dt, const T position_variance, const T angle_variance)
{
        const T dt_2 = power<2>(dt) / 2;
        const T dt_3 = power<3>(dt) / 6;
        const Matrix<8, 3, T> noise_transition{
                {dt_3,    0,    0},
                {dt_2,    0,    0},
                {  dt,    0,    0},
                {   0, dt_3,    0},
                {   0, dt_2,    0},
                {   0,   dt,    0},
                {   0,    0, dt_2},
                {   0,    0,   dt}
        };

        const T p = position_variance;
        const T a = angle_variance;
        const Matrix<3, 3, T> move_covariance{
                {p, 0, 0},
                {0, p, 0},
                {0, 0, a}
        };

        return noise_transition * move_covariance * noise_transition.transposed();
}

//

template <typename T>
Matrix<2, 2, T> position_r(const Vector<2, T>& position_variance)
{
        return make_diagonal_matrix(position_variance);
}

template <typename T>
Vector<2, T> position_h(const Vector<8, T>& x)
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
Matrix<3, 3, T> position_speed_r(const Vector<2, T>& position_variance, const T speed_variance)
{
        const Vector<2, T>& pv = position_variance;
        const T sv = speed_variance;
        return make_diagonal_matrix<3, T>({pv[0], pv[1], sv});
}

template <typename T>
Vector<3, T> position_speed_h(const Vector<8, T>& x)
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
Matrix<4, 4, T> position_speed_direction_r(
        const Vector<2, T>& position_variance,
        const T speed_variance,
        const T direction_variance)
{
        const Vector<2, T>& pv = position_variance;
        const T sv = speed_variance;
        const T dv = direction_variance;
        return make_diagonal_matrix<4, T>({pv[0], pv[1], sv, dv});
}

template <typename T>
Vector<4, T> position_speed_direction_h(const Vector<8, T>& x)
{
        // px = px
        // py = py
        // speed = sqrt(vx*vx + vy*vy)
        // angle = atan(vy, vx) + angle
        const T px = x[0];
        const T vx = x[1];
        const T py = x[3];
        const T vy = x[4];
        const T angle = x[6];
        return {
                px, // px
                py, // py
                std::sqrt(vx * vx + vy * vy), // speed
                std::atan2(vy, vx) + angle // angle
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
Matrix<3, 3, T> position_direction_r(const Vector<2, T>& position_variance, const T direction_variance)
{
        const Vector<2, T>& pv = position_variance;
        const T dv = direction_variance;
        return make_diagonal_matrix<3, T>({pv[0], pv[1], dv});
}

template <typename T>
Vector<3, T> position_direction_h(const Vector<8, T>& x)
{
        // px = px
        // py = py
        // angle = atan(vy, vx) + angle
        const T px = x[0];
        const T vx = x[1];
        const T py = x[3];
        const T vy = x[4];
        const T angle = x[6];
        return {
                px, // px
                py, // py
                std::atan2(vy, vx) + angle // angle
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
Matrix<2, 2, T> speed_direction_r(const T speed_variance, const T direction_variance)
{
        const T sv = speed_variance;
        const T dv = direction_variance;
        return make_diagonal_matrix<2, T>({sv, dv});
}

template <typename T>
Vector<2, T> speed_direction_h(const Vector<8, T>& x)
{
        // speed = sqrt(vx*vx + vy*vy)
        // angle = atan(vy, vx) + angle
        const T vx = x[1];
        const T vy = x[4];
        const T angle = x[6];
        return {
                std::sqrt(vx * vx + vy * vy), // speed
                std::atan2(vy, vx) + angle // angle
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
Matrix<1, 1, T> direction_r(const T direction_variance)
{
        const T dv = direction_variance;
        return {{dv}};
}

template <typename T>
Vector<1, T> direction_h(const Vector<8, T>& x)
{
        // angle = atan(vy, vx) + angle
        const T vx = x[1];
        const T vy = x[4];
        const T angle = x[6];
        return Vector<1, T>{
                std::atan2(vy, vx) + angle // angle
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
Matrix<1, 1, T> speed_r(const T speed_variance)
{
        const T sv = speed_variance;
        return {{sv}};
}

template <typename T>
Vector<1, T> speed_h(const Vector<8, T>& x)
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
class Filter final : public MoveFilter<T>
{
        const T sigma_points_alpha_;
        const T position_variance_;
        const T angle_variance_;
        std::optional<Ukf<8, T, SigmaPoints<8, T>>> filter_;

        [[nodiscard]] Vector<2, T> velocity() const
        {
                ASSERT(filter_);

                return {filter_->x()[1], filter_->x()[4]};
        }

        [[nodiscard]] Matrix<2, 2, T> velocity_p() const
        {
                ASSERT(filter_);

                return {
                        {filter_->p()(1, 1), filter_->p()(1, 4)},
                        {filter_->p()(4, 1), filter_->p()(4, 4)}
                };
        }

        void reset(
                const Vector<6, T>& position_velocity_acceleration,
                const Matrix<6, 6, T>& position_velocity_acceleration_p,
                const T angle) override
        {
                filter_.emplace(
                        SigmaPoints<8, T>(sigma_points_alpha_, SIGMA_POINTS_BETA<T>, SIGMA_POINTS_KAPPA<8, T>),
                        x(position_velocity_acceleration, angle), p(position_velocity_acceleration_p));
        }

        void predict(const T dt) override
        {
                ASSERT(filter_);

                filter_->predict(
                        [dt](const Vector<8, T>& x)
                        {
                                return f(dt, x);
                        },
                        q(dt, position_variance_, angle_variance_));
        }

        void update_position(const Measurement<2, T>& position) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_h<T>, position_r(position.variance), position.value, AddX(), position_residual<T>);
        }

        void update_position_speed(const Measurement<2, T>& position, const Measurement<1, T>& speed) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_speed_h<T>, position_speed_r(position.variance, speed.variance),
                        Vector<3, T>(position.value[0], position.value[1], speed.value), AddX(),
                        position_speed_residual<T>);
        }

        void update_position_speed_direction(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_speed_direction_h<T>,
                        position_speed_direction_r(position.variance, speed.variance, direction.variance),
                        Vector<4, T>(position.value[0], position.value[1], speed.value, direction.value), AddX(),
                        position_speed_direction_residual<T>);
        }

        void update_position_direction(const Measurement<2, T>& position, const Measurement<1, T>& direction) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_direction_h<T>, position_direction_r(position.variance, direction.variance),
                        Vector<3, T>(position.value[0], position.value[1], direction.value), AddX(),
                        position_direction_residual<T>);
        }

        void update_speed_direction(const Measurement<1, T>& speed, const Measurement<1, T>& direction) override
        {
                ASSERT(filter_);

                filter_->update(
                        speed_direction_h<T>, speed_direction_r(speed.variance, direction.variance),
                        Vector<2, T>(speed.value, direction.value), AddX(), speed_direction_residual<T>);
        }

        void update_direction(const Measurement<1, T>& direction) override
        {
                ASSERT(filter_);

                filter_->update(
                        direction_h<T>, direction_r(direction.variance), Vector<1, T>(direction.value), AddX(),
                        direction_residual<T>);
        }

        void update_speed(const Measurement<1, T>& speed) override
        {
                ASSERT(filter_);

                filter_->update(
                        speed_h<T>, speed_r(speed.variance), Vector<1, T>(speed.value), AddX(), speed_residual<T>);
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
                        {filter_->p()(0, 0), filter_->p()(0, 3)},
                        {filter_->p()(3, 0), filter_->p()(3, 3)}
                };
        }

        [[nodiscard]] T speed() const override
        {
                return velocity().norm();
        }

        [[nodiscard]] T speed_p() const override
        {
                return compute_speed_p(velocity(), velocity_p());
        }

        [[nodiscard]] T angle() const override
        {
                ASSERT(filter_);

                return filter_->x()[6];
        }

        [[nodiscard]] T angle_p() const override
        {
                ASSERT(filter_);

                return filter_->p()(6, 6);
        }

        [[nodiscard]] T angle_speed() const override
        {
                ASSERT(filter_);

                return filter_->x()[7];
        }

        [[nodiscard]] T angle_speed_p() const override
        {
                ASSERT(filter_);

                return filter_->p()(7, 7);
        }

public:
        Filter(const T sigma_points_alpha, const T position_variance, const T angle_variance)
                : sigma_points_alpha_(sigma_points_alpha),
                  position_variance_(position_variance),
                  angle_variance_(angle_variance)
        {
        }
};
}

template <typename T>
std::unique_ptr<MoveFilter<T>> create_move_filter_ukf(
        const T sigma_points_alpha,
        const T position_variance,
        const T angle_variance)
{
        return std::make_unique<Filter<T>>(sigma_points_alpha, position_variance, angle_variance);
}

#define TEMPLATE(T) template std::unique_ptr<MoveFilter<T>> create_move_filter_ukf(T, T, T);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
