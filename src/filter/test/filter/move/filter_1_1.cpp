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

#include "filter_1_1.h"

#include "../../../sigma_points.h"
#include "../../../ukf.h"
#include "../../utility/utility.h"

#include <src/com/angle.h>
#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/exponent.h>

#include <optional>

namespace ns::filter::test::move
{
namespace
{
template <typename T>
constexpr T SIGMA_POINTS_BETA = 2; // 2 for Gaussian
template <std::size_t N, typename T>
constexpr T SIGMA_POINTS_KAPPA = 3 - T{N};

template <typename T>
constexpr T INIT_ANGLE_SPEED = 0;
template <typename T>
constexpr T INIT_ANGLE_SPEED_VARIANCE = square(degrees_to_radians(1.0));

template <typename T>
Vector<6, T> x(const Vector<2, T>& position, const Vector<2, T>& velocity, const T angle)
{
        ASSERT(is_finite(position));
        ASSERT(is_finite(velocity));
        ASSERT(is_finite(angle));

        Vector<6, T> res;

        res[0] = position[0];
        res[1] = velocity[0];
        res[2] = position[1];
        res[3] = velocity[1];
        res[4] = angle;
        res[5] = INIT_ANGLE_SPEED<T>;

        return res;
}

template <typename T>
Matrix<6, 6, T> p(const Vector<2, T>& position_variance, const Vector<2, T>& velocity_variance, const T angle_variance)
{
        ASSERT(is_finite(position_variance));
        ASSERT(is_finite(velocity_variance));
        ASSERT(is_finite(angle_variance));

        Matrix<6, 6, T> res(0);

        res(0, 0) = position_variance[0];
        res(1, 1) = velocity_variance[0];
        res(2, 2) = position_variance[1];
        res(3, 3) = velocity_variance[1];
        res(4, 4) = angle_variance;
        res(5, 5) = INIT_ANGLE_SPEED_VARIANCE<T>;

        return res;
}

template <typename T>
Vector<6, T> x(const Vector<4, T>& position_velocity, const T angle)
{
        ASSERT(is_finite(position_velocity));

        Vector<6, T> res;

        res[0] = position_velocity[0];
        res[1] = position_velocity[1];
        res[2] = position_velocity[2];
        res[3] = position_velocity[3];
        res[4] = angle;
        res[5] = INIT_ANGLE_SPEED<T>;

        return res;
}

template <typename T>
Matrix<6, 6, T> p(const Matrix<4, 4, T>& position_velocity_p, const T angle_variance)
{
        ASSERT(is_finite(position_velocity_p));

        Matrix<6, 6, T> res(0);

        for (std::size_t r = 0; r < 4; ++r)
        {
                for (std::size_t c = 0; c < 4; ++c)
                {
                        res(r, c) = position_velocity_p(r, c);
                }
        }

        res(4, 4) = angle_variance;
        res(5, 5) = INIT_ANGLE_SPEED_VARIANCE<T>;

        return res;
}

struct AddX final
{
        template <typename T>
        [[nodiscard]] Vector<6, T> operator()(const Vector<6, T>& a, const Vector<6, T>& b) const
        {
                Vector<6, T> res = a + b;
                res[4] = normalize_angle(res[4]);
                return res;
        }
};

template <typename T>
Vector<6, T> f(const T dt, const Vector<6, T>& x)
{
        const T px = x[0];
        const T vx = x[1];
        const T py = x[2];
        const T vy = x[3];
        const T angle = x[4];
        const T angle_v = x[5];

        return {
                px + dt * vx, // px
                vx, // vx
                py + dt * vy, // py
                vy, // vy
                angle + dt * angle_v, // angle
                angle_v // angle_v
        };
}

template <typename T>
constexpr Matrix<6, 6, T> q(const T dt, const T position_variance, const T angle_variance)
{
        const T dt_2 = power<2>(dt) / 2;
        const Matrix<6, 3, T> noise_transition{
                {dt_2,    0,    0},
                {  dt,    0,    0},
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
Vector<2, T> position_h(const Vector<6, T>& x)
{
        // px = px
        // py = py
        return {x[0], x[2]};
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
Vector<3, T> position_speed_h(const Vector<6, T>& x)
{
        // px = px
        // py = py
        // speed = sqrt(vx*vx + vy*vy)
        const T px = x[0];
        const T vx = x[1];
        const T py = x[2];
        const T vy = x[3];
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
        const Vector<1, T>& speed_variance,
        const Vector<1, T>& direction_variance)
{
        const Vector<2, T>& pv = position_variance;
        const Vector<1, T>& sv = speed_variance;
        const Vector<1, T>& dv = direction_variance;
        return make_diagonal_matrix<4, T>({pv[0], pv[1], sv[0], dv[0]});
}

template <typename T>
Vector<4, T> position_speed_direction_h(const Vector<6, T>& x)
{
        // px = px
        // py = py
        // speed = sqrt(vx*vx + vy*vy)
        // angle = atan(vy, vx) + angle
        const T px = x[0];
        const T vx = x[1];
        const T py = x[2];
        const T vy = x[3];
        const T angle = x[4];
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
Matrix<3, 3, T> position_direction_r(const Vector<2, T>& position_variance, const Vector<1, T>& direction_variance)
{
        const Vector<2, T>& pv = position_variance;
        const Vector<1, T>& dv = direction_variance;
        return make_diagonal_matrix<3, T>({pv[0], pv[1], dv[0]});
}

template <typename T>
Vector<3, T> position_direction_h(const Vector<6, T>& x)
{
        // px = px
        // py = py
        // angle = atan(vy, vx) + angle
        const T px = x[0];
        const T vx = x[1];
        const T py = x[2];
        const T vy = x[3];
        const T angle = x[4];
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
Matrix<2, 2, T> speed_direction_r(const Vector<1, T>& speed_variance, const Vector<1, T>& direction_variance)
{
        const Vector<1, T>& sv = speed_variance;
        const Vector<1, T>& dv = direction_variance;
        return make_diagonal_matrix<2, T>({sv[0], dv[0]});
}

template <typename T>
Vector<2, T> speed_direction_h(const Vector<6, T>& x)
{
        // speed = sqrt(vx*vx + vy*vy)
        // angle = atan(vy, vx) + angle
        const T vx = x[1];
        const T vy = x[3];
        const T angle = x[4];
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
Matrix<1, 1, T> direction_r(const Vector<1, T>& direction_variance)
{
        const Vector<1, T>& dv = direction_variance;
        return {{dv[0]}};
}

template <typename T>
Vector<1, T> direction_h(const Vector<6, T>& x)
{
        // angle = atan(vy, vx) + angle
        const T vx = x[1];
        const T vy = x[3];
        const T angle = x[4];
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
Matrix<1, 1, T> speed_r(const Vector<1, T>& speed_variance)
{
        const Vector<1, T>& sv = speed_variance;
        return {{sv[0]}};
}

template <typename T>
Vector<1, T> speed_h(const Vector<6, T>& x)
{
        // speed = sqrt(vx*vx + vy*vy)
        const T vx = x[1];
        const T vy = x[3];
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
class Filter final : public Filter11<T>
{
        static constexpr bool NORMALIZED_INNOVATION{false};
        static constexpr bool LIKELIHOOD{false};

        const T sigma_points_alpha_;
        const T position_variance_;
        const T angle_variance_;
        std::optional<Ukf<6, T, SigmaPoints<6, T>>> filter_;

        [[nodiscard]] Vector<2, T> velocity() const
        {
                ASSERT(filter_);

                return {filter_->x()[1], filter_->x()[3]};
        }

        [[nodiscard]] Matrix<2, 2, T> velocity_p() const
        {
                ASSERT(filter_);

                return {
                        {filter_->p()(1, 1), filter_->p()(1, 3)},
                        {filter_->p()(3, 1), filter_->p()(3, 3)}
                };
        }

        void reset(
                const Vector<2, T>& position,
                const Vector<2, T>& position_variance,
                const Vector<2, T>& velocity,
                const Vector<2, T>& velocity_variance,
                const T angle,
                const T angle_variance) override
        {
                filter_.emplace(
                        SigmaPoints<6, T>(sigma_points_alpha_, SIGMA_POINTS_BETA<T>, SIGMA_POINTS_KAPPA<6, T>),
                        x(position, velocity, angle), p(position_variance, velocity_variance, angle_variance));
        }

        void reset(
                const Vector<4, T>& position_velocity,
                const Matrix<4, 4, T>& position_velocity_p,
                const T angle,
                const T angle_variance) override
        {
                filter_.emplace(
                        SigmaPoints<6, T>(sigma_points_alpha_, SIGMA_POINTS_BETA<T>, SIGMA_POINTS_KAPPA<6, T>),
                        x(position_velocity, angle), p(position_velocity_p, angle_variance));
        }

        void predict(const T dt) override
        {
                ASSERT(filter_);

                filter_->predict(
                        [dt](const Vector<6, T>& x)
                        {
                                return f(dt, x);
                        },
                        q(dt, position_variance_, angle_variance_));
        }

        void update_position(const Measurement<2, T>& position, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_h<T>, position_r(position.variance), position.value, AddX(), position_residual<T>,
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
                        Vector<3, T>(position.value[0], position.value[1], speed.value[0]), AddX(),
                        position_speed_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
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
                        Vector<4, T>(position.value[0], position.value[1], speed.value[0], direction.value[0]), AddX(),
                        position_speed_direction_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_position_direction(
                const Measurement<2, T>& position,
                const Measurement<1, T>& direction,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_direction_h<T>, position_direction_r(position.variance, direction.variance),
                        Vector<3, T>(position.value[0], position.value[1], direction.value[0]), AddX(),
                        position_direction_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_speed_direction(
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        speed_direction_h<T>, speed_direction_r(speed.variance, direction.variance),
                        Vector<2, T>(speed.value[0], direction.value[0]), AddX(), speed_direction_residual<T>, gate,
                        NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_direction(const Measurement<1, T>& direction, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        direction_h<T>, direction_r(direction.variance), Vector<1, T>(direction.value), AddX(),
                        direction_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_speed(const Measurement<1, T>& speed, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        speed_h<T>, speed_r(speed.variance), Vector<1, T>(speed.value), AddX(), speed_residual<T>, gate,
                        NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        [[nodiscard]] Vector<2, T> position() const override
        {
                ASSERT(filter_);

                return {filter_->x()[0], filter_->x()[2]};
        }

        [[nodiscard]] Matrix<2, 2, T> position_p() const override
        {
                ASSERT(filter_);

                return {
                        {filter_->p()(0, 0), filter_->p()(0, 2)},
                        {filter_->p()(2, 0), filter_->p()(2, 2)}
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

                return filter_->x()[4];
        }

        [[nodiscard]] T angle_p() const override
        {
                ASSERT(filter_);

                return filter_->p()(4, 4);
        }

        [[nodiscard]] T angle_speed() const override
        {
                ASSERT(filter_);

                return filter_->x()[5];
        }

        [[nodiscard]] T angle_speed_p() const override
        {
                ASSERT(filter_);

                return filter_->p()(5, 5);
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
std::unique_ptr<Filter11<T>> create_filter_1_1(
        const T sigma_points_alpha,
        const T position_variance,
        const T angle_variance)
{
        return std::make_unique<Filter<T>>(sigma_points_alpha, position_variance, angle_variance);
}

#define TEMPLATE(T) template std::unique_ptr<Filter11<T>> create_filter_1_1(T, T, T);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
