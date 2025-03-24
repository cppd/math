/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "filter_ekf_conv.h"
#include "filter_ekf_model.h"
#include "init.h"

#include <src/com/error.h>
#include <src/filter/core/ekf.h>
#include <src/filter/core/update_info.h>
#include <src/filter/filters/com/utility.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/filters/noise_model.h>
#include <src/filter/settings/instantiation.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <memory>
#include <optional>

namespace ns::filter::filters::acceleration
{
namespace model = filter_ekf_model;
namespace conv = filter_ekf_conv;

namespace
{
constexpr bool NORMALIZED_INNOVATION{true};
constexpr bool LIKELIHOOD{false};

template <typename T>
class Filter final : public FilterEkf<T>
{
        std::optional<core::Ekf<9, T>> filter_;

        void reset(
                const numerical::Vector<4, T>& position_velocity,
                const numerical::Matrix<4, 4, T>& position_velocity_p,
                const Init<T>& init) override
        {
                filter_.emplace(model::x(position_velocity, init), model::p(position_velocity_p, init));
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

                const numerical::Matrix<9, 9, T> f = model::f(dt);
                filter_->predict(
                        [&](const numerical::Vector<9, T>& x)
                        {
                                return f * x;
                        },
                        [&](const numerical::Vector<9, T>& /*x*/)
                        {
                                return f;
                        },
                        model::q(dt, position_noise_model, angle_noise_model, angle_r_noise_model),
                        fading_memory_alpha);
        }

        core::UpdateInfo<2, T> update_position(const Measurement<2, T>& position, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::position_h<T>, model::position_hj<T>, model::position_r(position.variance),
                        position.value, model::add_x<T>, model::position_residual<T>, gate, NORMALIZED_INNOVATION,
                        LIKELIHOOD);
        }

        core::UpdateInfo<3, T> update_position_speed(
                const Measurement<2, T>& position,
                const Measurement<1, T>& speed,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::position_speed_h<T>, model::position_speed_hj<T>,
                        model::position_speed_r(position.variance, speed.variance),
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
                        model::position_speed_direction_acceleration_hj<T>,
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
                        model::position_speed_direction_h<T>, model::position_speed_direction_hj<T>,
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
                        model::position_speed_acceleration_h<T>, model::position_speed_acceleration_hj<T>,
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
                        model::position_direction_acceleration_h<T>, model::position_direction_acceleration_hj<T>,
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
                        model::position_direction_h<T>, model::position_direction_hj<T>,
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
                        model::position_acceleration_h<T>, model::position_acceleration_hj<T>,
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
                        model::speed_direction_acceleration_h<T>, model::speed_direction_acceleration_hj<T>,
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
                        model::speed_direction_h<T>, model::speed_direction_hj<T>,
                        model::speed_direction_r(speed.variance, direction.variance),
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
                        model::direction_acceleration_h<T>, model::direction_acceleration_hj<T>,
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
                        model::acceleration_h<T>, model::acceleration_hj<T>,
                        model::acceleration_r(acceleration.variance), acceleration.value, model::add_x<T>,
                        model::acceleration_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        core::UpdateInfo<1, T> update_direction(const Measurement<1, T>& direction, const std::optional<T> gate)
                override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::direction_h<T>, model::direction_hj<T>, model::direction_r(direction.variance),
                        numerical::Vector<1, T>(direction.value), model::add_x<T>, model::direction_residual<T>, gate,
                        NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        core::UpdateInfo<1, T> update_speed(const Measurement<1, T>& speed, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::speed_h<T>, model::speed_hj<T>, model::speed_r(speed.variance),
                        numerical::Vector<1, T>(speed.value), model::add_x<T>, model::speed_residual<T>, gate,
                        NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        core::UpdateInfo<3, T> update_speed_acceleration(
                const Measurement<1, T>& speed,
                const Measurement<2, T>& acceleration,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                return filter_->update(
                        model::speed_acceleration_h<T>, model::speed_acceleration_hj<T>,
                        model::speed_acceleration_r(speed.variance, acceleration.variance),
                        numerical::Vector<3, T>(speed.value[0], acceleration.value[0], acceleration.value[1]),
                        model::add_x<T>, model::speed_acceleration_residual<T>, gate, NORMALIZED_INNOVATION,
                        LIKELIHOOD);
        }

        [[nodiscard]] const numerical::Vector<9, T>& x() const
        {
                ASSERT(filter_);

                return filter_->x();
        }

        [[nodiscard]] const numerical::Matrix<9, 9, T>& p() const
        {
                ASSERT(filter_);

                return filter_->p();
        }

        [[nodiscard]] numerical::Vector<2, T> position() const override
        {
                return conv::position(x());
        }

        [[nodiscard]] numerical::Matrix<2, 2, T> position_p() const override
        {
                return conv::position_p(p());
        }

        [[nodiscard]] T speed() const override
        {
                return conv::speed(x());
        }

        [[nodiscard]] T speed_p() const override
        {
                return conv::speed_p(x(), p());
        }

        [[nodiscard]] T angle() const override
        {
                return conv::angle(x());
        }

        [[nodiscard]] T angle_p() const override
        {
                return conv::angle_p(p());
        }

        [[nodiscard]] T angle_speed() const override
        {
                return conv::angle_speed(x());
        }

        [[nodiscard]] T angle_speed_p() const override
        {
                return conv::angle_speed_p(p());
        }

        [[nodiscard]] T angle_r() const override
        {
                return conv::angle_r(x());
        }

        [[nodiscard]] T angle_r_p() const override
        {
                return conv::angle_r_p(p());
        }
};
}

template <typename T>
std::unique_ptr<FilterEkf<T>> create_filter_ekf()
{
        return std::make_unique<Filter<T>>();
}

#define TEMPLATE(T) template std::unique_ptr<FilterEkf<T>> create_filter_ekf();

FILTER_TEMPLATE_INSTANTIATION_T(TEMPLATE)
}
