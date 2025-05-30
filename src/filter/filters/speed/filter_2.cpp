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

#include "filter_2.h"

#include "filter_2_conv.h"
#include "filter_2_measurement.h"
#include "filter_2_model.h"
#include "init.h"

#include <src/com/error.h>
#include <src/filter/core/sigma_points.h>
#include <src/filter/core/ukf.h>
#include <src/filter/core/update_info.h>
#include <src/filter/filters/com/utility.h>
#include <src/filter/filters/measurement.h>
#include <src/filter/filters/noise_model.h>
#include <src/filter/settings/instantiation.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <memory>
#include <optional>

namespace ns::filter::filters::speed
{
namespace conv = filter_2_conv;
namespace measurement = filter_2_measurement;
namespace model = filter_2_model;

namespace
{
constexpr bool NORMALIZED_INNOVATION{true};
constexpr bool LIKELIHOOD{false};

template <std::size_t N, typename T>
class Filter final : public Filter2<N, T>
{
        const T sigma_points_alpha_;
        std::optional<core::Ukf<3 * N, T, core::SigmaPoints<3 * N, T>>> filter_;

        void reset(
                const numerical::Vector<2 * N, T>& position_velocity,
                const numerical::Matrix<2 * N, 2 * N, T>& position_velocity_p,
                const Init<T>& init) override
        {
                filter_.emplace(
                        core::create_sigma_points<3 * N, T>(sigma_points_alpha_),
                        model::x<N, T>(position_velocity, init), model::p<N, T>(position_velocity_p, init));
        }

        void predict(const T dt, const NoiseModel<T>& noise_model, const T fading_memory_alpha) override
        {
                ASSERT(filter_);
                ASSERT(com::check_dt(dt));

                filter_->predict(
                        [dt](const numerical::Vector<3 * N, T>& x)
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
                        measurement::position_h<N, T>, measurement::position_r(position.variance), position.value,
                        model::add_x<N, T>, measurement::position_residual<N, T>, gate, NORMALIZED_INNOVATION,
                        LIKELIHOOD);
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
                        measurement::position_speed_h<N, T>,
                        measurement::position_speed_r(position.variance, speed.variance),
                        measurement::position_speed_z(position.value, speed.value), model::add_x<N, T>,
                        measurement::position_speed_residual<N, T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        core::UpdateInfo<1, T> update_speed(const Measurement<1, T>& speed, const std::optional<T> gate) override
        {
                ASSERT(filter_);
                ASSERT(com::check_variance(speed.variance));

                return filter_->update(
                        measurement::speed_h<N, T>, measurement::speed_r(speed.variance), speed.value,
                        model::add_x<N, T>, measurement::speed_residual<T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        [[nodiscard]] const numerical::Vector<3 * N, T>& x() const
        {
                ASSERT(filter_);

                return filter_->x();
        }

        [[nodiscard]] const numerical::Matrix<3 * N, 3 * N, T>& p() const
        {
                ASSERT(filter_);

                return filter_->p();
        }

        [[nodiscard]] numerical::Vector<N, T> position() const override
        {
                return conv::position(x());
        }

        [[nodiscard]] numerical::Matrix<N, N, T> position_p() const override
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

public:
        explicit Filter(const T sigma_points_alpha)
                : sigma_points_alpha_(sigma_points_alpha)
        {
        }
};
}

template <std::size_t N, typename T>
std::unique_ptr<Filter2<N, T>> create_filter_2(const T sigma_points_alpha)
{
        return std::make_unique<Filter<N, T>>(sigma_points_alpha);
}

#define TEMPLATE(N, T) template std::unique_ptr<Filter2<(N), T>> create_filter_2(T);

FILTER_TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
