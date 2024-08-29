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

#include <cstddef>
#include <memory>
#include <optional>

namespace ns::filter::filters::speed
{
namespace model = filter_1_model;

namespace
{
constexpr bool NORMALIZED_INNOVATION{true};
constexpr bool LIKELIHOOD{false};

template <std::size_t N, typename T>
class Filter final : public Filter1<N, T>
{
        const T sigma_points_alpha_;
        std::optional<core::Ukf<2 * N, T, core::SigmaPoints<2 * N, T>>> filter_;

        [[nodiscard]] numerical::Vector<N, T> velocity() const
        {
                ASSERT(filter_);

                return numerical::slice<1, 2>(filter_->x());
        }

        [[nodiscard]] numerical::Matrix<N, N, T> velocity_p() const
        {
                ASSERT(filter_);

                return numerical::slice<1, 2>(filter_->p());
        }

        void reset(
                const numerical::Vector<2 * N, T>& position_velocity,
                const numerical::Matrix<2 * N, 2 * N, T>& position_velocity_p,
                const Init<T>& /*init*/) override
        {
                filter_.emplace(
                        core::create_sigma_points<2 * N, T>(sigma_points_alpha_), model::x(position_velocity),
                        model::p(position_velocity_p));
        }

        void predict(const T dt, const NoiseModel<T>& noise_model, const T fading_memory_alpha) override
        {
                ASSERT(filter_);
                ASSERT(com::check_dt(dt));

                filter_->predict(
                        [dt](const numerical::Vector<2 * N, T>& x)
                        {
                                return model::f<N, T>(dt, x);
                        },
                        model::q<N, T>(dt, noise_model), fading_memory_alpha);
        }

        core::UpdateInfo<N, T> update_position(const Measurement<N, T>& position, const std::optional<T> gate) override
        {
                ASSERT(filter_);
                ASSERT(com::check_variance(position.variance));

                return filter_->update(
                        model::position_h<N, T>, model::position_r(position.variance), position.value,
                        model::add_x<N, T>, model::position_residual<N, T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        core::UpdateInfo<N + 1, T> update_position_speed(
                const Measurement<N, T>& position,
                const Measurement<1, T>& speed,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);
                ASSERT(com::check_variance(position.variance));
                ASSERT(com::check_variance(speed.variance));

                return filter_->update(
                        model::position_speed_h<N, T>, model::position_speed_r(position.variance, speed.variance),
                        model::position_speed_z(position.value, speed.value), model::add_x<N, T>,
                        model::position_speed_residual<N, T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        core::UpdateInfo<1, T> update_speed(const Measurement<1, T>& speed, const std::optional<T> gate) override
        {
                ASSERT(filter_);
                ASSERT(com::check_variance(speed.variance));

                return filter_->update(
                        model::speed_h<N, T>, model::speed_r(speed.variance), speed.value, model::add_x<N, T>,
                        model::speed_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        [[nodiscard]] numerical::Vector<N, T> position() const override
        {
                ASSERT(filter_);

                return numerical::slice<0, 2>(filter_->x());
        }

        [[nodiscard]] numerical::Matrix<N, N, T> position_p() const override
        {
                ASSERT(filter_);

                return numerical::slice<0, 2>(filter_->p());
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
        explicit Filter(const T sigma_points_alpha)
                : sigma_points_alpha_(sigma_points_alpha)
        {
        }
};
}

template <std::size_t N, typename T>
std::unique_ptr<Filter1<N, T>> create_filter_1(const T sigma_points_alpha)
{
        return std::make_unique<Filter<N, T>>(sigma_points_alpha);
}

#define TEMPLATE(N, T) template std::unique_ptr<Filter1<(N), T>> create_filter_1(T);

FILTER_TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
