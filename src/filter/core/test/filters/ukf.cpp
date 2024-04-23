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

#include "ukf.h"

#include "noise_model.h"

#include <src/com/error.h>
#include <src/com/variant.h>
#include <src/filter/core/models.h>
#include <src/filter/core/sigma_points.h>
#include <src/filter/core/ukf.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <memory>
#include <optional>

namespace ns::filter::core::test::filters
{
namespace
{
struct Add final
{
        template <std::size_t N, typename T>
        [[nodiscard]] numerical::Vector<N, T> operator()(
                const numerical::Vector<N, T>& a,
                const numerical::Vector<N, T>& b) const
        {
                return a + b;
        }
};

struct Residual final
{
        template <std::size_t N, typename T>
        [[nodiscard]] numerical::Vector<N, T> operator()(
                const numerical::Vector<N, T>& a,
                const numerical::Vector<N, T>& b) const
        {
                return a - b;
        }
};

template <typename T>
numerical::Vector<2, T> f(const T dt, const numerical::Vector<2, T>& x)
{
        return {x[0] + dt * x[1], x[1]};
}

template <typename T>
numerical::Matrix<2, 2, T> q(const T dt, const NoiseModel<T>& noise_model)
{
        const auto visitors = Visitors{
                [&](const ContinuousNoiseModel<T>& model)
                {
                        return continuous_white_noise<2, T>(dt, model.spectral_density);
                },
                [&](const DiscreteNoiseModel<T>& model)
                {
                        const T dt_2 = power<2>(dt) / 2;
                        const numerical::Matrix<2, 1, T> noise_transition{{dt_2}, {dt}};
                        const numerical::Matrix<1, 1, T> covariance{{model.variance}};
                        return noise_transition * covariance * noise_transition.transposed();
                }};
        return std::visit(visitors, noise_model);
}

//

template <typename T>
numerical::Matrix<1, 1, T> position_r(const T position_variance)
{
        return {{position_variance}};
}

template <typename T>
numerical::Vector<1, T> position_h(const numerical::Vector<2, T>& x)
{
        // x = x[0]
        return numerical::Vector<1, T>(x[0]);
}

//

template <typename T>
numerical::Matrix<2, 2, T> position_speed_r(const T position_variance, const T speed_variance)
{
        return {
                {position_variance,              0},
                {                0, speed_variance}
        };
}

template <typename T>
numerical::Vector<2, T> position_speed_h(const numerical::Vector<2, T>& x)
{
        // x = x[0]
        // v = x[1]
        return x;
}

//

template <typename T>
numerical::Matrix<1, 1, T> speed_r(const T speed_variance)
{
        return {{speed_variance}};
}

template <typename T>
numerical::Vector<1, T> speed_h(const numerical::Vector<2, T>& x)
{
        // v = x[1]
        return numerical::Vector<1, T>(x[1]);
}

//

template <typename T>
class Filter final : public FilterUkf<T>
{
        static constexpr bool NORMALIZED_INNOVATION{true};
        static constexpr bool LIKELIHOOD{true};

        static constexpr T SIGMA_POINTS_ALPHA = 0.1;

        std::optional<Ukf<2, T, SigmaPoints<2, T>>> filter_;

        void reset(const numerical::Vector<2, T>& x, const numerical::Matrix<2, 2, T>& p) override
        {
                filter_.emplace(create_sigma_points<2, T>(SIGMA_POINTS_ALPHA), x, p);
        }

        void predict(const T dt, const NoiseModel<T>& noise_model, const T fading_memory_alpha) override
        {
                ASSERT(filter_);

                filter_->predict(
                        [dt](const numerical::Vector<2, T>& x)
                        {
                                return f(dt, x);
                        },
                        q(dt, noise_model), fading_memory_alpha);
        }

        void update_position(const T position, const T position_variance, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_h<T>, position_r<T>(position_variance), numerical::Vector<1, T>(position), Add(),
                        Residual(), gate, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_position_speed(
                const T position,
                const T position_variance,
                const T speed,
                const T speed_variance,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_speed_h<T>, position_speed_r<T>(position_variance, speed_variance),
                        numerical::Vector<2, T>(position, speed), Add(), Residual(), gate, NORMALIZED_INNOVATION,
                        LIKELIHOOD);
        }

        void update_speed(const T speed, const T speed_variance, const std::optional<T> gate) override
        {
                ASSERT(filter_);

                filter_->update(
                        speed_h<T>, speed_r<T>(speed_variance), numerical::Vector<1, T>(speed), Add(), Residual(), gate,
                        NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        [[nodiscard]] T position() const override
        {
                ASSERT(filter_);

                return filter_->x()[0];
        }

        [[nodiscard]] T position_p() const override
        {
                ASSERT(filter_);

                return filter_->p()[0, 0];
        }

        [[nodiscard]] numerical::Vector<2, T> position_speed() const override
        {
                ASSERT(filter_);

                return filter_->x();
        }

        [[nodiscard]] numerical::Matrix<2, 2, T> position_speed_p() const override
        {
                ASSERT(filter_);

                return filter_->p();
        }

        [[nodiscard]] T speed() const override
        {
                ASSERT(filter_);

                return filter_->x()[1];
        }

        [[nodiscard]] T speed_p() const override
        {
                ASSERT(filter_);

                return filter_->p()[1, 1];
        }

public:
        Filter() = default;
};
}

template <typename T>
[[nodiscard]] std::unique_ptr<FilterUkf<T>> create_filter_ukf()
{
        return std::make_unique<Filter<T>>();
}

#define INSTANTIATION(T) template std::unique_ptr<FilterUkf<T>> create_filter_ukf();

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
