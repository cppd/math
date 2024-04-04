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

#include <src/com/error.h>
#include <src/filter/core/sigma_points.h>
#include <src/filter/core/ukf.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace ns::filter::core::test
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
numerical::Matrix<2, 2, T> q(const T dt, const T process_variance)
{
        const T dt_2 = power<2>(dt) / 2;
        const numerical::Matrix<2, 1, T> noise_transition{{dt_2}, {dt}};
        const numerical::Matrix<1, 1, T> covariance{{process_variance}};
        return noise_transition * covariance * noise_transition.transposed();
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
class Filter final : public TestUkf<T>
{
        static constexpr std::optional<T> GATE{};
        static constexpr bool NORMALIZED_INNOVATION{true};
        static constexpr bool LIKELIHOOD{true};

        static constexpr T SIGMA_POINTS_ALPHA = 0.1;

        std::optional<Ukf<2, T, SigmaPoints<2, T>>> filter_;

        void reset(const numerical::Vector<2, T>& x, const numerical::Matrix<2, 2, T>& p) override
        {
                filter_.emplace(create_sigma_points<2, T>(SIGMA_POINTS_ALPHA), x, p);
        }

        void predict(const T dt, const T process_variance) override
        {
                ASSERT(filter_);

                filter_->predict(
                        [dt](const numerical::Vector<2, T>& x)
                        {
                                return f(dt, x);
                        },
                        q(dt, process_variance));
        }

        void update_position(const T position, const T position_variance) override
        {
                ASSERT(filter_);

                filter_->update(
                        position_h<T>, position_r<T>(position_variance), numerical::Vector<1, T>(position), Add(),
                        Residual(), GATE, NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_position_speed(const T position, const T position_variance, const T speed, const T speed_variance)
                override
        {
                ASSERT(filter_);

                filter_->update(
                        position_speed_h<T>, position_speed_r<T>(position_variance, speed_variance),
                        numerical::Vector<2, T>(position, speed), Add(), Residual(), GATE, NORMALIZED_INNOVATION,
                        LIKELIHOOD);
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

        [[nodiscard]] std::string name() const override
        {
                return "UKF";
        }

public:
        Filter() = default;
};
}

template <typename T>
[[nodiscard]] std::unique_ptr<TestUkf<T>> create_test_ukf()
{
        return std::make_unique<Filter<T>>();
}

#define INSTANTIATION(T) template std::unique_ptr<TestUkf<T>> create_test_ukf();

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
