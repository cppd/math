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

#include "filter_1_model.h"
#include "init.h"

#include <src/com/error.h>
#include <src/filter/core/sigma_points.h>
#include <src/filter/core/ukf.h>
#include <src/filter/core/update_info.h>
#include <src/filter/filters/com/utility.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/filters/noise_model.h>
#include <src/filter/utility/instantiation.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <memory>
#include <optional>

namespace ns::filter::filters::acceleration
{
namespace
{
constexpr bool NORMALIZED_INNOVATION{true};
constexpr bool LIKELIHOOD{false};

namespace model = filter_1_model;

template <typename T>
class Filter final : public Filter1<T>
{
        const T sigma_points_alpha_;
        std::optional<core::Ukf<9, T, core::SigmaPoints<9, T>>> filter_;

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
                filter_.emplace(
                        core::create_sigma_points<9, T>(sigma_points_alpha_), model::x(position_velocity, init),
                        model::p(position_velocity_p, init));
        }

        void predict(
                const T dt,
                const NoiseModel<T>& position_noise_model,
                const NoiseModel<T>& angle_noise_model,
                const NoiseModel<T>& angle_r_noise_model,
                const T fading_memory_alpha) override
        {
                ASSERT(filter_);
                ASSERT(com::check_dt(dt));

                filter_->predict(
                        [dt](const numerical::Vector<9, T>& x)
                        {
                                return model::f(dt, x);
                        },
                        model::q(dt, position_noise_model, angle_noise_model, angle_r_noise_model),
                        fading_memory_alpha);
        }

        core::UpdateInfo<2, T> update_position(const Measurement<2, T>& position, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::position_h<T>, model::position_r(position.variance), position.value, model::add_x<T>,
                        model::position_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        core::UpdateInfo<3, T> update_position_speed(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::position_speed_h<T>, model::position_speed_r(position.variance, speed.variance),
                        numerical::Vector<3, T>(position.value[0], position.value[1], speed.value[0]), model::add_x<T>,
                        model::position_speed_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        core::UpdateInfo<6, T> update_position_speed_direction_acceleration(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::position_speed_direction_acceleration_h<T>,
                        model::position_speed_direction_acceleration_r(
                                position.variance, speed.variance, direction.variance, acceleration.variance),
                        numerical::Vector<6, T>(
                                position.value[0], position.value[1], speed.value[0], direction.value[0],
                                acceleration.value[0], acceleration.value[1]),
                        model::add_x<T>, model::position_speed_direction_acceleration_residual<T>, gate,
                        NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        core::UpdateInfo<4, T> update_position_speed_direction(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::position_speed_direction_h<T>,
                        model::position_speed_direction_r(position.variance, speed.variance, direction.variance),
                        numerical::Vector<4, T>(
                                position.value[0], position.value[1], speed.value[0], direction.value[0]),
                        model::add_x<T>, model::position_speed_direction_residual<T>, gate, NORMALIZED_INNOVATION,
                        LIKELIHOOD);
        }

        core::UpdateInfo<5, T> update_position_speed_acceleration(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::position_speed_acceleration_h<T>,
                        model::position_speed_acceleration_r(position.variance, speed.variance, acceleration.variance),
                        numerical::Vector<5, T>(
                                position.value[0], position.value[1], speed.value[0], acceleration.value[0],
                                acceleration.value[1]),
                        model::add_x<T>, model::position_speed_acceleration_residual<T>, gate, NORMALIZED_INNOVATION,
                        LIKELIHOOD);
        }

        core::UpdateInfo<5, T> update_position_direction_acceleration(
                const Measurement<2, T>& position,
                const Measurement<1, T>& direction,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::position_direction_acceleration_h<T>,
                        model::position_direction_acceleration_r(
                                position.variance, direction.variance, acceleration.variance),
                        numerical::Vector<5, T>(
                                position.value[0], position.value[1], direction.value[0], acceleration.value[0],
                                acceleration.value[1]),
                        model::add_x<T>, model::position_direction_acceleration_residual<T>, gate,
                        NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        core::UpdateInfo<3, T> update_position_direction(
                const Measurement<2, T>& position,
                const Measurement<1, T>& direction,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::position_direction_h<T>,
                        model::position_direction_r(position.variance, direction.variance),
                        numerical::Vector<3, T>(position.value[0], position.value[1], direction.value[0]),
                        model::add_x<T>, model::position_direction_residual<T>, gate, NORMALIZED_INNOVATION,
                        LIKELIHOOD);
        }

        core::UpdateInfo<4, T> update_position_acceleration(
                const Measurement<2, T>& position,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::position_acceleration_h<T>,
                        model::position_acceleration_r(position.variance, acceleration.variance),
                        numerical::Vector<4, T>(
                                position.value[0], position.value[1], acceleration.value[0], acceleration.value[1]),
                        model::add_x<T>, model::position_acceleration_residual<T>, gate, NORMALIZED_INNOVATION,
                        LIKELIHOOD);
        }

        core::UpdateInfo<4, T> update_speed_direction_acceleration(
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::speed_direction_acceleration_h<T>,
                        model::speed_direction_acceleration_r(
                                speed.variance, direction.variance, acceleration.variance),
                        numerical::Vector<4, T>(
                                speed.value[0], direction.value[0], acceleration.value[0], acceleration.value[1]),
                        model::add_x<T>, model::speed_direction_acceleration_residual<T>, gate, NORMALIZED_INNOVATION,
                        LIKELIHOOD);
        }

        core::UpdateInfo<2, T> update_speed_direction(
                const Measurement<1, T>& speed,
                const Measurement<1, T>& direction,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::speed_direction_h<T>, model::speed_direction_r(speed.variance, direction.variance),
                        numerical::Vector<2, T>(speed.value[0], direction.value[0]), model::add_x<T>,
                        model::speed_direction_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        core::UpdateInfo<3, T> update_direction_acceleration(
                const Measurement<1, T>& direction,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::direction_acceleration_h<T>,
                        model::direction_acceleration_r(direction.variance, acceleration.variance),
                        numerical::Vector<3, T>(direction.value[0], acceleration.value[0], acceleration.value[1]),
                        model::add_x<T>, model::direction_acceleration_residual<T>, gate, NORMALIZED_INNOVATION,
                        LIKELIHOOD);
        }

        core::UpdateInfo<2, T> update_acceleration(const Measurement<2, T>& acceleration, const std::optional<T> gate)
                override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::acceleration_h<T>, model::acceleration_r(acceleration.variance), acceleration.value,
                        model::add_x<T>, model::acceleration_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        core::UpdateInfo<1, T> update_direction(const Measurement<1, T>& direction, const std::optional<T> gate)
                override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::direction_h<T>, model::direction_r(direction.variance),
                        numerical::Vector<1, T>(direction.value), model::add_x<T>, model::direction_residual<T>, gate,
                        NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        core::UpdateInfo<1, T> update_speed(const Measurement<1, T>& speed, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::speed_h<T>, model::speed_r(speed.variance), numerical::Vector<1, T>(speed.value),
                        model::add_x<T>, model::speed_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        core::UpdateInfo<3, T> update_speed_acceleration(
                const Measurement<1, T>& speed,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::speed_acceleration_h<T>,
                        model::speed_acceleration_r(speed.variance, acceleration.variance),
                        numerical::Vector<3, T>(speed.value[0], acceleration.value[0], acceleration.value[1]),
                        model::add_x<T>, model::speed_acceleration_residual<T>, gate, NORMALIZED_INNOVATION,
                        LIKELIHOOD);
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
        explicit Filter(const T sigma_points_alpha)
                : sigma_points_alpha_(sigma_points_alpha)
        {
        }
};
}

template <typename T>
std::unique_ptr<Filter1<T>> create_filter_1(const T sigma_points_alpha)
{
        return std::make_unique<Filter<T>>(sigma_points_alpha);
}

#define TEMPLATE(T) template std::unique_ptr<Filter1<T>> create_filter_1(T);

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)
}
