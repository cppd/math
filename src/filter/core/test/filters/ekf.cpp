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

#include "ekf.h"

#include "ekf_conv.h"
#include "ekf_model.h"
#include "noise_model.h"

#include <src/com/error.h>
#include <src/filter/core/ekf.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <memory>
#include <optional>

namespace ns::filter::core::test::filters
{
namespace model = ekf_model;
namespace conv = ekf_conv;

namespace
{
template <typename T, bool H_INFINITY>
class Filter final : public FilterEkf<T, H_INFINITY>
{
        static constexpr bool NORMALIZED_INNOVATION{true};
        static constexpr bool LIKELIHOOD{true};

        static constexpr T THETA{0.01L};

        std::optional<Ekf<2, T>> filter_;

        void filter_update(
                const auto& h,
                const auto& hj,
                const auto& r,
                const auto& z,
                const auto& residual,
                const auto& gate)
        {
                ASSERT(filter_);
                if constexpr (H_INFINITY)
                {
                        filter_->update(
                                h, hj, r, z, model::add_x<T>, residual, gate, NORMALIZED_INNOVATION, LIKELIHOOD, THETA);
                }
                else
                {
                        filter_->update(
                                h, hj, r, z, model::add_x<T>, residual, gate, NORMALIZED_INNOVATION, LIKELIHOOD);
                }
        }

        void reset(const numerical::Vector<2, T>& x, const numerical::Matrix<2, 2, T>& p) override
        {
                filter_.emplace(x, p);
        }

        numerical::Matrix<2, 2, T> predict(const T dt, const NoiseModel<T>& noise_model, const T fading_memory_alpha)
                override
        {
                ASSERT(filter_);

                const numerical::Matrix<2, 2, T> f = model::f(dt);

                filter_->predict(
                        [&](const numerical::Vector<2, T>& x)
                        {
                                return f * x;
                        },
                        [&](const numerical::Vector<2, T>& /*x*/)
                        {
                                return f;
                        },
                        model::q(dt, noise_model), fading_memory_alpha);

                return f;
        }

        void update_position(const T position, const T position_variance, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_update(
                        model::position_h<T>, model::position_hj<T>, model::position_r<T>(position_variance),
                        numerical::Vector<1, T>(position), model::position_residual<T>, gate);
        }

        void update_position_speed(
                const T position,
                const T position_variance,
                const T speed,
                const T speed_variance,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_update(
                        model::position_speed_h<T>, model::position_speed_hj<T>,
                        model::position_speed_r<T>(position_variance, speed_variance),
                        numerical::Vector<2, T>(position, speed), model::position_speed_residual<T>, gate);
        }

        void update_speed(const T speed, const T speed_variance, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_update(
                        model::speed_h<T>, model::speed_hj<T>, model::speed_r<T>(speed_variance),
                        numerical::Vector<1, T>(speed), model::speed_residual<T>, gate);
        }

        [[nodiscard]] const numerical::Vector<2, T>& x() const
        {
                ASSERT(filter_);

                return filter_->x();
        }

        [[nodiscard]] const numerical::Matrix<2, 2, T>& p() const
        {
                ASSERT(filter_);

                return filter_->p();
        }

        [[nodiscard]] T position() const override
        {
                return conv::position(x());
        }

        [[nodiscard]] T position_p() const override
        {
                return conv::position_p(p());
        }

        [[nodiscard]] numerical::Vector<2, T> position_speed() const override
        {
                return conv::position_speed(x());
        }

        [[nodiscard]] numerical::Matrix<2, 2, T> position_speed_p() const override
        {
                return conv::position_speed_p(p());
        }

        [[nodiscard]] T speed() const override
        {
                return conv::speed(x());
        }

        [[nodiscard]] T speed_p() const override
        {
                return conv::speed_p(p());
        }

public:
        Filter() = default;
};
}

template <typename T, bool H_INFINITY>
[[nodiscard]] std::unique_ptr<FilterEkf<T, H_INFINITY>> create_filter_ekf()
{
        return std::make_unique<Filter<T, H_INFINITY>>();
}

#define INSTANTIATION(T)                                                   \
        template std::unique_ptr<FilterEkf<T, false>> create_filter_ekf(); \
        template std::unique_ptr<FilterEkf<T, true>> create_filter_ekf();

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
