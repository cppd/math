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

#include "filter_2.h"

#include "../../../sigma_points.h"
#include "../../../ukf.h"
#include "../../utility/utility.h"

#include <src/com/error.h>
#include <src/com/exponent.h>

#include <optional>

namespace ns::filter::test::filter::speed
{
namespace
{
template <typename T>
constexpr T SIGMA_POINTS_BETA = 2; // 2 for Gaussian
template <std::size_t N, typename T>
constexpr T SIGMA_POINTS_KAPPA = 3 - T{N};

template <typename T>
constexpr T INIT_ACCELERATION = 0;
template <typename T>
constexpr T INIT_ACCELERATION_VARIANCE = square(10);

template <typename T>
Vector<6, T> x(const Vector<2, T>& position, const Vector<2, T>& velocity, const Vector<2, T>& acceleration)
{
        ASSERT(is_finite(position));
        ASSERT(is_finite(velocity));
        ASSERT(is_finite(acceleration));

        Vector<6, T> res;

        res[0] = position[0];
        res[1] = velocity[0];
        res[2] = acceleration[0];
        res[3] = position[1];
        res[4] = velocity[1];
        res[5] = acceleration[1];

        return res;
}

template <typename T>
Matrix<6, 6, T> p(
        const Vector<2, T>& position_variance,
        const Vector<2, T>& velocity_variance,
        const Vector<2, T>& acceleration_variance)
{
        ASSERT(is_finite(position_variance));
        ASSERT(is_finite(velocity_variance));
        ASSERT(is_finite(acceleration_variance));

        Matrix<6, 6, T> res(0);

        res(0, 0) = position_variance[0];
        res(1, 1) = velocity_variance[0];
        res(2, 2) = acceleration_variance[0];
        res(3, 3) = position_variance[1];
        res(4, 4) = velocity_variance[1];
        res(5, 5) = acceleration_variance[1];

        return res;
}

template <typename T>
Vector<6, T> x(const Vector<6, T>& position_velocity_acceleration)
{
        ASSERT(is_finite(position_velocity_acceleration));

        Vector<6, T> res;

        for (std::size_t i = 0; i < 6; ++i)
        {
                res[i] = position_velocity_acceleration[i];
        }

        return res;
}

template <typename T>
Matrix<6, 6, T> p(const Matrix<6, 6, T>& position_velocity_acceleration_p)
{
        ASSERT(is_finite(position_velocity_acceleration_p));

        Matrix<6, 6, T> res(0);

        for (std::size_t r = 0; r < 6; ++r)
        {
                for (std::size_t c = 0; c < 6; ++c)
                {
                        res(r, c) = position_velocity_acceleration_p(r, c);
                }
        }

        return res;
}

template <typename T>
Vector<6, T> x(const Vector<4, T>& position_velocity)
{
        ASSERT(is_finite(position_velocity));

        Vector<6, T> res;

        res[0] = position_velocity[0];
        res[1] = position_velocity[1];
        res[2] = INIT_ACCELERATION<T>;
        res[3] = position_velocity[2];
        res[4] = position_velocity[3];
        res[5] = INIT_ACCELERATION<T>;

        return res;
}

template <typename T>
Matrix<6, 6, T> p(const Matrix<4, 4, T>& position_velocity_p)
{
        ASSERT(is_finite(position_velocity_p));

        const Matrix<4, 4, T>& p = position_velocity_p;
        static constexpr std::size_t N = 2;

        Matrix<6, 6, T> res(0);

        for (std::size_t r = 0; r < N; ++r)
        {
                for (std::size_t i = 0; i < 2; ++i)
                {
                        for (std::size_t c = 0; c < N; ++c)
                        {
                                for (std::size_t j = 0; j < 2; ++j)
                                {
                                        res(3 * r + i, 3 * c + j) = p(2 * r + i, 2 * c + j);
                                }
                        }
                }
        }

        res(2, 2) = INIT_ACCELERATION_VARIANCE<T>;
        res(5, 5) = INIT_ACCELERATION_VARIANCE<T>;

        return res;
}

struct AddX final
{
        template <typename T>
        [[nodiscard]] Vector<6, T> operator()(const Vector<6, T>& a, const Vector<6, T>& b) const
        {
                return a + b;
        }
};

template <typename T>
Vector<6, T> f(const T dt, const Vector<6, T>& x)
{
        const T dt_2 = square(dt) / 2;

        const T px = x[0];
        const T vx = x[1];
        const T ax = x[2];
        const T py = x[3];
        const T vy = x[4];
        const T ay = x[5];

        return {
                px + dt * vx + dt_2 * ax, // px
                vx + dt * ax, // vx
                ax, // ax
                py + dt * vy + dt_2 * ay, // py
                vy + dt * ay, // vy
                ay // ay
        };
}

template <typename T>
constexpr Matrix<6, 6, T> q(const T dt, const T position_variance)
{
        const T dt_2 = power<2>(dt) / 2;
        const T dt_3 = power<3>(dt) / 6;
        const Matrix<6, 2, T> noise_transition{
                {dt_3,    0},
                {dt_2,    0},
                {  dt,    0},
                {   0, dt_3},
                {   0, dt_2},
                {   0,   dt}
        };

        const T p = position_variance;
        const Matrix<2, 2, T> move_covariance{
                {p, 0},
                {0, p},
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
Vector<3, T> position_speed_h(const Vector<6, T>& x)
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
class Filter final : public Filter2<T>
{
        static constexpr bool NORMALIZED_INNOVATION{false};
        static constexpr bool LIKELIHOOD{false};

        const T sigma_points_alpha_;
        const T position_variance_;
        std::optional<Ukf<6, T, SigmaPoints<6, T>>> filter_;

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
                const Vector<2, T>& position,
                const Vector<2, T>& position_variance,
                const Vector<2, T>& velocity,
                const Vector<2, T>& velocity_variance,
                const Vector<2, T>& acceleration,
                const Vector<2, T>& acceleration_variance) override
        {
                filter_.emplace(
                        SigmaPoints<6, T>(sigma_points_alpha_, SIGMA_POINTS_BETA<T>, SIGMA_POINTS_KAPPA<6, T>),
                        x(position, velocity, acceleration),
                        p(position_variance, velocity_variance, acceleration_variance));
        }

        void reset(
                const Vector<6, T>& position_velocity_acceleration,
                const Matrix<6, 6, T>& position_velocity_acceleration_p) override
        {
                filter_.emplace(
                        SigmaPoints<6, T>(sigma_points_alpha_, SIGMA_POINTS_BETA<T>, SIGMA_POINTS_KAPPA<6, T>),
                        x(position_velocity_acceleration), p(position_velocity_acceleration_p));
        }

        void reset(const Vector<4, T>& position_velocity, const Matrix<4, 4, T>& position_velocity_p) override
        {
                filter_.emplace(
                        SigmaPoints<6, T>(sigma_points_alpha_, SIGMA_POINTS_BETA<T>, SIGMA_POINTS_KAPPA<6, T>),
                        x(position_velocity), p(position_velocity_p));
        }

        void predict(const T dt) override
        {
                ASSERT(filter_);
                ASSERT(utility::check_dt(dt));

                filter_->predict(
                        [dt](const Vector<6, T>& x)
                        {
                                return f(dt, x);
                        },
                        q(dt, position_variance_));
        }

        void update_position(const Measurement<2, T>& position, const std::optional<T> gate) override
        {
                ASSERT(filter_);
                ASSERT(utility::check_variance(position.variance));

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
                ASSERT(utility::check_variance(position.variance));
                ASSERT(utility::check_variance(speed.variance));

                filter_->update(
                        position_speed_h<T>, position_speed_r(position.variance, speed.variance),
                        Vector<3, T>(position.value[0], position.value[1], speed.value[0]), AddX(),
                        position_speed_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_speed(const Measurement<1, T>& speed, const std::optional<T> gate) override
        {
                ASSERT(filter_);
                ASSERT(utility::check_variance(speed.variance));

                filter_->update(
                        speed_h<T>, speed_r(speed.variance), Vector<1, T>(speed.value), AddX(), speed_residual<T>, gate,
                        NORMALIZED_INNOVATION, LIKELIHOOD);
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

                return filter_->p()(6, 6);
        }

public:
        Filter(const T sigma_points_alpha, const T position_variance)
                : sigma_points_alpha_(sigma_points_alpha),
                  position_variance_(position_variance)
        {
        }
};
}

template <typename T>
std::unique_ptr<Filter2<T>> create_filter_2(const T sigma_points_alpha, const T position_variance)
{
        return std::make_unique<Filter<T>>(sigma_points_alpha, position_variance);
}

#define TEMPLATE(T) template std::unique_ptr<Filter2<T>> create_filter_2(T, T);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
