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

#include "filter_2_model.h"
#include "init.h"

#include <src/com/error.h>
#include <src/filter/core/ekf.h>
#include <src/filter/core/update_info.h>
#include <src/filter/filters/com/utility.h>
#include <src/filter/filters/noise_model.h>
#include <src/filter/settings/instantiation.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <memory>
#include <optional>

namespace ns::filter::filters::position
{
namespace model = filter_2_model;

namespace
{
constexpr bool NORMALIZED_INNOVATION{true};
constexpr bool LIKELIHOOD{false};

template <std::size_t N, typename T>
class FilterImpl final : public Filter2<N, T>
{
        const std::optional<T> theta_;
        std::optional<core::Ekf<3 * N, T>> filter_;

        void reset(
                const numerical::Vector<N, T>& position,
                const numerical::Vector<N, T>& variance,
                const Init<T>& init) override
        {
                filter_.emplace(model::x(position, init), model::p(variance, init));
        }

        void predict(const T dt, const NoiseModel<T>& noise_model, const T fading_memory_alpha) override
        {
                ASSERT(filter_);
                ASSERT(com::check_dt(dt));

                const numerical::Matrix<3 * N, 3 * N, T> f = model::f<N, T>(dt);
                filter_->predict(
                        [&](const numerical::Vector<3 * N, T>& x)
                        {
                                return f * x;
                        },
                        [&](const numerical::Vector<3 * N, T>& /*x*/)
                        {
                                return f;
                        },
                        model::q<N, T>(dt, noise_model), fading_memory_alpha);
        }

        [[nodiscard]] core::UpdateInfo<N, T> update(
                const numerical::Vector<N, T>& position,
                const numerical::Vector<N, T>& variance,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);
                ASSERT(is_finite(position));
                ASSERT(com::check_variance(variance));

                if (theta_)
                {
                        return filter_->update(
                                model::position_h<N, T>, model::position_hj<N, T>, model::position_r(variance),
                                position, model::add_x<N, T>, model::position_residual<N, T>, gate,
                                NORMALIZED_INNOVATION, LIKELIHOOD, *theta_);
                }

                return filter_->update(
                        model::position_h<N, T>, model::position_hj<N, T>, model::position_r(variance), position,
                        model::add_x<N, T>, model::position_residual<N, T>, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        [[nodiscard]] numerical::Vector<N, T> position() const override
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

        [[nodiscard]] numerical::Vector<N, T> velocity() const override
        {
                ASSERT(filter_);

                return numerical::slice<1, 3>(filter_->x());
        }

        [[nodiscard]] numerical::Matrix<N, N, T> velocity_p() const override
        {
                ASSERT(filter_);

                return numerical::slice<1, 3>(filter_->p());
        }

        [[nodiscard]] numerical::Vector<2 * N, T> position_velocity() const override
        {
                ASSERT(filter_);

                const numerical::Vector<3 * N, T>& x = filter_->x();
                numerical::Vector<2 * N, T> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        for (std::size_t j = 0; j < 2; ++j)
                        {
                                res[2 * i + j] = x[3 * i + j];
                        }
                }
                return res;
        }

        [[nodiscard]] numerical::Matrix<2 * N, 2 * N, T> position_velocity_p() const override
        {
                ASSERT(filter_);

                const numerical::Matrix<3 * N, 3 * N, T>& p = filter_->p();
                numerical::Matrix<2 * N, 2 * N, T> res;
                for (std::size_t r = 0; r < N; ++r)
                {
                        for (std::size_t i = 0; i < 2; ++i)
                        {
                                for (std::size_t c = 0; c < N; ++c)
                                {
                                        for (std::size_t j = 0; j < 2; ++j)
                                        {
                                                res[2 * r + i, 2 * c + j] = p[3 * r + i, 3 * c + j];
                                        }
                                }
                        }
                }
                return res;
        }

        [[nodiscard]] numerical::Vector<3 * N, T> position_velocity_acceleration() const override
        {
                ASSERT(filter_);

                return filter_->x();
        }

        [[nodiscard]] numerical::Matrix<3 * N, 3 * N, T> position_velocity_acceleration_p() const override
        {
                ASSERT(filter_);

                return filter_->p();
        }

public:
        explicit FilterImpl(const T theta)
                : theta_(theta)
        {
                ASSERT(theta_ >= 0);
        }
};
}

template <std::size_t N, typename T>
std::unique_ptr<Filter2<N, T>> create_filter_2(const T theta)
{
        return std::make_unique<FilterImpl<N, T>>(theta);
}

#define TEMPLATE(N, T) template std::unique_ptr<Filter2<(N), T>> create_filter_2<(N), T>(T);

FILTER_TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
