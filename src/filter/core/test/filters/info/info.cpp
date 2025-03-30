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

#include "info.h"

#include "info_conv.h"
#include "info_model.h"

#include <src/com/error.h>
#include <src/filter/core/info.h>
#include <src/filter/core/test/filters/noise_model.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <memory>
#include <optional>

namespace ns::filter::core::test::filters::info
{
namespace
{
template <typename T>
class Filter final : public FilterInfo<T>
{
        std::optional<Info<2, T>> filter_;

        void reset(const numerical::Vector<2, T>& x, const numerical::Matrix<2, 2, T>& i) override
        {
                filter_.emplace(x, i);
        }

        numerical::Matrix<2, 2, T> predict(const T dt, const NoiseModel<T>& noise_model, const T fading_memory_alpha)
                override
        {
                ASSERT(filter_);

                const numerical::Matrix<2, 2, T> f = model::f(dt);
                const numerical::Matrix<2, 2, T> q_inv = model::q(dt, noise_model).inversed();

                filter_->predict(
                        [&](const numerical::Vector<2, T>& x)
                        {
                                return f * x;
                        },
                        [&](const numerical::Vector<2, T>& /*x*/)
                        {
                                return f;
                        },
                        q_inv, fading_memory_alpha);

                return f;
        }

        void update_position(const T position, const T position_variance, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                const auto r_inv = model::position_r<T>(position_variance).inversed();

                filter_->update(
                        model::position_h<T>, model::position_hj<T>, r_inv, numerical::Vector<1, T>(position),
                        model::add_x<T>, model::position_residual<T>, gate);
        }

        void update_position_speed(
                const T position,
                const T position_variance,
                const T speed,
                const T speed_variance,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                const auto r_inv = model::position_speed_r<T>(position_variance, speed_variance).inversed();

                filter_->update(
                        model::position_speed_h<T>, model::position_speed_hj<T>, r_inv,
                        numerical::Vector<2, T>(position, speed), model::add_x<T>, model::position_speed_residual<T>,
                        gate);
        }

        void update_speed(const T speed, const T speed_variance, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                const auto r_inv = model::speed_r<T>(speed_variance).inversed();

                filter_->update(
                        model::speed_h<T>, model::speed_hj<T>, r_inv, numerical::Vector<1, T>(speed), model::add_x<T>,
                        model::speed_residual<T>, gate);
        }

        [[nodiscard]] const numerical::Vector<2, T>& x() const
        {
                ASSERT(filter_);

                return filter_->x();
        }

        [[nodiscard]] const std::optional<numerical::Matrix<2, 2, T>>& p() const
        {
                ASSERT(filter_);

                return filter_->p();
        }

        [[nodiscard]] const numerical::Matrix<2, 2, T>& i() const
        {
                ASSERT(filter_);

                return filter_->i();
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
                return conv::position_speed_p(p(), i());
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

template <typename T>
[[nodiscard]] std::unique_ptr<FilterInfo<T>> create_filter_info()
{
        return std::make_unique<Filter<T>>();
}

#define INSTANTIATION(T) template std::unique_ptr<FilterInfo<T>> create_filter_info();

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
