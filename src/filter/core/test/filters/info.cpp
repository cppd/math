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

#include "info.h"

#include "ekf_model.h"
#include "noise_model.h"

#include <src/com/error.h>
#include <src/filter/core/info.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <memory>
#include <optional>

namespace ns::filter::core::test::filters
{
namespace
{
template <typename T>
class Filter final : public FilterInfo<T>
{
        std::optional<Info<2, T>> filter_;

        void filter_update(const auto& h, const auto& hj, const auto& r, const auto& z)
        {
                ASSERT(filter_);

                filter_->update(h, hj, r, z, ekf_model::Add(), ekf_model::Residual());
        }

        void reset(const numerical::Vector<2, T>& x, const numerical::Matrix<2, 2, T>& i) override
        {
                filter_.emplace(x, i);
        }

        void predict(const T dt, const NoiseModel<T>& noise_model, const T fading_memory_alpha) override
        {
                ASSERT(filter_);

                const numerical::Matrix<2, 2, T> f_matrix = ekf_model::f(dt);
                const numerical::Matrix<2, 2, T> q_inv = ekf_model::q(dt, noise_model).inversed();

                filter_->predict(
                        [&](const numerical::Vector<2, T>& x)
                        {
                                return f_matrix * x;
                        },
                        [&](const numerical::Vector<2, T>& /*x*/)
                        {
                                return f_matrix;
                        },
                        q_inv, fading_memory_alpha);
        }

        void update_position(const T position, const T position_variance) override
        {
                ASSERT(filter_);

                const auto r_inv = ekf_model::position_r<T>(position_variance).inversed();

                filter_update(
                        ekf_model::position_h<T>, ekf_model::position_hj<T>, r_inv, numerical::Vector<1, T>(position));
        }

        void update_position_speed(const T position, const T position_variance, const T speed, const T speed_variance)
                override
        {
                ASSERT(filter_);

                const auto r_inv = ekf_model::position_speed_r<T>(position_variance, speed_variance).inversed();

                filter_update(
                        ekf_model::position_speed_h<T>, ekf_model::position_speed_hj<T>, r_inv,
                        numerical::Vector<2, T>(position, speed));
        }

        void update_speed(const T speed, const T speed_variance) override
        {
                ASSERT(filter_);

                const auto r_inv = ekf_model::position_r<T>(speed_variance).inversed();

                filter_update(ekf_model::speed_h<T>, ekf_model::speed_hj<T>, r_inv, numerical::Vector<1, T>(speed));
        }

        [[nodiscard]] T position() const override
        {
                ASSERT(filter_);

                return filter_->x()[0];
        }

        [[nodiscard]] T position_p() const override
        {
                ASSERT(filter_);

                const auto& p = filter_->p();
                ASSERT(p);
                return (*p)[0, 0];
        }

        [[nodiscard]] numerical::Vector<2, T> position_speed() const override
        {
                ASSERT(filter_);

                return filter_->x();
        }

        [[nodiscard]] numerical::Matrix<2, 2, T> position_speed_p() const override
        {
                ASSERT(filter_);

                const auto& p = filter_->p();
                ASSERT(p);
                return *p;
        }

        [[nodiscard]] T speed() const override
        {
                ASSERT(filter_);

                return filter_->x()[1];
        }

        [[nodiscard]] T speed_p() const override
        {
                ASSERT(filter_);

                const auto& p = filter_->p();
                ASSERT(p);
                return (*p)[1, 1];
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
