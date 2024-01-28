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

#include "filter_2.h"

#include "init.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/filter/core/sigma_points.h>
#include <src/filter/core/ukf.h>
#include <src/filter/filters/com/utility.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/utility/instantiation.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <memory>
#include <optional>

namespace ns::filter::filters::speed
{
namespace
{
template <std::size_t N, typename T>
Vector<3 * N, T> x(const Vector<N, T>& position, const Vector<N, T>& velocity, const Vector<N, T>& acceleration)
{
        ASSERT(is_finite(position));
        ASSERT(is_finite(velocity));
        ASSERT(is_finite(acceleration));

        Vector<3 * N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                const std::size_t b = 3 * i;
                res[b + 0] = position[i];
                res[b + 1] = velocity[i];
                res[b + 2] = acceleration[i];
        }
        return res;
}

template <std::size_t N, typename T>
numerical::Matrix<3 * N, 3 * N, T> p(
        const Vector<N, T>& position_variance,
        const Vector<N, T>& velocity_variance,
        const Vector<N, T>& acceleration_variance)
{
        ASSERT(is_finite(position_variance));
        ASSERT(is_finite(velocity_variance));
        ASSERT(is_finite(acceleration_variance));

        numerical::Matrix<3 * N, 3 * N, T> res(0);
        for (std::size_t i = 0; i < N; ++i)
        {
                const std::size_t b = 3 * i;
                res[b + 0, b + 0] = position_variance[i];
                res[b + 1, b + 1] = velocity_variance[i];
                res[b + 2, b + 2] = acceleration_variance[i];
        }
        return res;
}

template <std::size_t N, typename T>
Vector<N, T> x(const Vector<N, T>& position_velocity_acceleration)
{
        ASSERT(is_finite(position_velocity_acceleration));

        return position_velocity_acceleration;
}

template <std::size_t N, typename T>
numerical::Matrix<N, N, T> p(const numerical::Matrix<N, N, T>& position_velocity_acceleration_p)
{
        ASSERT(is_finite(position_velocity_acceleration_p));

        return position_velocity_acceleration_p;
}

template <std::size_t N, typename T>
Vector<3 * N, T> x(const Vector<2 * N, T>& position_velocity, const Init<T>& init)
{
        ASSERT(is_finite(position_velocity));

        Vector<3 * N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                const std::size_t a = 3 * i;
                const std::size_t b = 2 * i;
                res[a + 0] = position_velocity[b + 0];
                res[a + 1] = position_velocity[b + 1];
                res[a + 2] = init.acceleration;
        }
        return res;
}

template <std::size_t N, typename T>
numerical::Matrix<3 * N, 3 * N, T> p(const numerical::Matrix<2 * N, 2 * N, T>& position_velocity_p, const Init<T>& init)
{
        ASSERT(is_finite(position_velocity_p));

        const numerical::Matrix<2 * N, 2 * N, T>& p = position_velocity_p;

        numerical::Matrix<3 * N, 3 * N, T> res(0);

        for (std::size_t r = 0; r < N; ++r)
        {
                const std::size_t ar = 3 * r;
                const std::size_t br = 2 * r;
                for (std::size_t c = 0; c < N; ++c)
                {
                        const std::size_t ac = 3 * c;
                        const std::size_t bc = 2 * c;
                        res[ar + 0, ac + 0] = p[br + 0, bc + 0];
                        res[ar + 0, ac + 1] = p[br + 0, bc + 1];
                        res[ar + 1, ac + 0] = p[br + 1, bc + 0];
                        res[ar + 1, ac + 1] = p[br + 1, bc + 1];
                }
                res[ar + 2, ar + 2] = init.acceleration_variance;
        }

        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] Vector<N, T> add_x(const Vector<N, T>& a, const Vector<N, T>& b)
{
        return a + b;
}

template <std::size_t N, typename T>
Vector<3 * N, T> f(const T dt, const Vector<3 * N, T>& x)
{
        const T dt_2 = square(dt) / 2;

        Vector<3 * N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                const std::size_t b = 3 * i;
                const T p = x[b + 0];
                const T v = x[b + 1];
                const T a = x[b + 2];
                res[b + 0] = p + dt * v + dt_2 * a;
                res[b + 1] = v + dt * a;
                res[b + 2] = a;
        }
        return res;
}

template <std::size_t N, typename T>
constexpr numerical::Matrix<3 * N, 3 * N, T> q(const T dt, const T position_variance)
{
        const T dt_2 = power<2>(dt) / 2;
        const T dt_3 = power<3>(dt) / 6;

        const numerical::Matrix<3 * N, N, T> noise_transition =
                block_diagonal<N>(numerical::Matrix<3, 1, T>{{dt_3}, {dt_2}, {dt}});
        const numerical::Matrix<N, N, T> process_covariance =
                numerical::make_diagonal_matrix(Vector<N, T>(position_variance));

        return noise_transition * process_covariance * noise_transition.transposed();
}

//

template <std::size_t N, typename T>
Vector<N, T> position_z(const Vector<N, T>& position)
{
        return position;
}

template <std::size_t N, typename T>
numerical::Matrix<N, N, T> position_r(const Vector<N, T>& position_variance)
{
        return numerical::make_diagonal_matrix(position_variance);
}

template <std::size_t N, typename T>
Vector<N, T> position_h(const Vector<3 * N, T>& x)
{
        Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = x[3 * i];
        }
        return res;
}

template <std::size_t N, typename T>
Vector<N, T> position_residual(const Vector<N, T>& a, const Vector<N, T>& b)
{
        return a - b;
}

//

template <std::size_t N, typename T>
Vector<N + 1, T> position_speed_z(const Vector<N, T>& position, const Vector<1, T>& speed)
{
        Vector<N + 1, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = position[i];
        }
        res[N] = speed[0];
        return res;
}

template <std::size_t N, typename T>
numerical::Matrix<N + 1, N + 1, T> position_speed_r(
        const Vector<N, T>& position_variance,
        const Vector<1, T>& speed_variance)
{
        numerical::Matrix<N + 1, N + 1, T> res(0);
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i, i] = position_variance[i];
        }
        res[N, N] = speed_variance[0];
        return res;
}

template <std::size_t N, typename T>
Vector<N + 1, T> position_speed_h(const Vector<3 * N, T>& x)
{
        Vector<N + 1, T> res;
        Vector<N, T> velocity;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = x[3 * i];
                velocity[i] = x[3 * i + 1];
        }
        res[N] = velocity.norm();
        return res;
}

template <std::size_t N, typename T>
Vector<N, T> position_speed_residual(const Vector<N, T>& a, const Vector<N, T>& b)
{
        return a - b;
}

//

template <typename T>
Vector<1, T> speed_z(const Vector<1, T>& speed)
{
        return speed;
}

template <typename T>
numerical::Matrix<1, 1, T> speed_r(const Vector<1, T>& speed_variance)
{
        return {{speed_variance[0]}};
}

template <std::size_t N, typename T>
Vector<1, T> speed_h(const Vector<3 * N, T>& x)
{
        Vector<N, T> velocity;
        for (std::size_t i = 0; i < N; ++i)
        {
                velocity[i] = x[3 * i + 1];
        }
        return Vector<1, T>{velocity.norm()};
}

template <typename T>
Vector<1, T> speed_residual(const Vector<1, T>& a, const Vector<1, T>& b)
{
        return a - b;
}

//

template <std::size_t N, typename T>
class Filter final : public Filter2<N, T>
{
        static constexpr bool NORMALIZED_INNOVATION{false};
        static constexpr bool LIKELIHOOD{false};

        const T sigma_points_alpha_;
        const T position_variance_;
        std::optional<core::Ukf<3 * N, T, core::SigmaPoints<3 * N, T>>> filter_;

        [[nodiscard]] Vector<N, T> velocity() const
        {
                ASSERT(filter_);

                return numerical::slice<1, 3>(filter_->x());
        }

        [[nodiscard]] numerical::Matrix<N, N, T> velocity_p() const
        {
                ASSERT(filter_);

                return numerical::slice<1, 3>(filter_->p());
        }

        void reset(
                const Vector<N, T>& position,
                const Vector<N, T>& position_variance,
                const Vector<N, T>& velocity,
                const Vector<N, T>& velocity_variance,
                const Vector<N, T>& acceleration,
                const Vector<N, T>& acceleration_variance) override
        {
                filter_.emplace(
                        core::create_sigma_points<3 * N, T>(sigma_points_alpha_), x(position, velocity, acceleration),
                        p(position_variance, velocity_variance, acceleration_variance));
        }

        void reset(
                const Vector<3 * N, T>& position_velocity_acceleration,
                const numerical::Matrix<3 * N, 3 * N, T>& position_velocity_acceleration_p) override
        {
                filter_.emplace(
                        core::create_sigma_points<3 * N, T>(sigma_points_alpha_), x(position_velocity_acceleration),
                        p(position_velocity_acceleration_p));
        }

        void reset(
                const Vector<2 * N, T>& position_velocity,
                const numerical::Matrix<2 * N, 2 * N, T>& position_velocity_p,
                const Init<T>& init) override
        {
                filter_.emplace(
                        core::create_sigma_points<3 * N, T>(sigma_points_alpha_), x<N, T>(position_velocity, init),
                        p<N, T>(position_velocity_p, init));
        }

        void predict(const T dt) override
        {
                ASSERT(filter_);
                ASSERT(com::check_dt(dt));

                filter_->predict(
                        [dt](const Vector<3 * N, T>& x)
                        {
                                return f<N, T>(dt, x);
                        },
                        q<N, T>(dt, position_variance_));
        }

        void update_position(const Measurement<N, T>& position, const std::optional<T> gate) override
        {
                ASSERT(filter_);
                ASSERT(com::check_variance(position.variance));

                filter_->update(
                        position_h<N, T>, position_r(position.variance), position_z(position.value), add_x<3 * N, T>,
                        position_residual<N, T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_position_speed(
                const Measurement<N, T>& position,
                const Measurement<1, T>& speed,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);
                ASSERT(com::check_variance(position.variance));
                ASSERT(com::check_variance(speed.variance));

                filter_->update(
                        position_speed_h<N, T>, position_speed_r(position.variance, speed.variance),
                        position_speed_z(position.value, speed.value), add_x<3 * N, T>,
                        position_speed_residual<N + 1, T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_speed(const Measurement<1, T>& speed, const std::optional<T> gate) override
        {
                ASSERT(filter_);
                ASSERT(com::check_variance(speed.variance));

                filter_->update(
                        speed_h<N, T>, speed_r(speed.variance), speed_z(speed.value), add_x<3 * N, T>,
                        speed_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        [[nodiscard]] Vector<N, T> position() const override
        {
                ASSERT(filter_);

                return numerical::slice<0, 3>(filter_->x());
        }

        [[nodiscard]] numerical::Matrix<N, N, T> position_p() const override
        {
                ASSERT(filter_);

                return numerical::slice<0, 3>(filter_->p());
        }

        [[nodiscard]] T speed() const override
        {
                return velocity().norm();
        }

        [[nodiscard]] T speed_p() const override
        {
                return com::compute_speed_p(velocity(), velocity_p());
        }

public:
        Filter(const T sigma_points_alpha, const T position_variance)
                : sigma_points_alpha_(sigma_points_alpha),
                  position_variance_(position_variance)
        {
        }
};
}

template <std::size_t N, typename T>
std::unique_ptr<Filter2<N, T>> create_filter_2(const T sigma_points_alpha, const T position_variance)
{
        return std::make_unique<Filter<N, T>>(sigma_points_alpha, position_variance);
}

#define TEMPLATE(N, T) template std::unique_ptr<Filter2<(N), T>> create_filter_2(T, T);

FILTER_TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
