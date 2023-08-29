/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "position_filter_lkf_0.h"

#include "../../ekf.h"
#include "../utility.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <optional>

namespace ns::filter::test
{
namespace
{
template <std::size_t N, typename T>
Vector<N, T> init_x(const Vector<N, T>& position)
{
        ASSERT(is_finite(position));

        return position;
}

template <std::size_t N, typename T>
Matrix<N, N, T> init_p(const Vector<N, T>& position_variance)
{
        ASSERT(is_finite(position_variance));

        Matrix<N, N, T> res(0);
        for (std::size_t i = 0; i < N; ++i)
        {
                res(i, i) = position_variance[i];
        }
        return res;
}

struct AddX final
{
        template <std::size_t N, typename T>
        [[nodiscard]] Vector<N, T> operator()(const Vector<N, T>& a, const Vector<N, T>& b) const
        {
                return a + b;
        }
};

template <std::size_t N, typename T>
Matrix<N, N, T> f_matrix(const T /*dt*/)
{
        return block_diagonal<N>(Matrix<1, 1, T>{
                {1},
        });
}

template <std::size_t N, typename T>
Matrix<N, N, T> q(const T dt, const T process_variance)
{
        const Matrix<N, N, T> noise_transition = block_diagonal<N>(Matrix<1, 1, T>{{dt}});
        const Matrix<N, N, T> process_covariance = make_diagonal_matrix(Vector<N, T>(process_variance));

        return noise_transition * process_covariance * noise_transition.transposed();
}

template <std::size_t N, typename T>
Matrix<N, N, T> position_r(const Vector<N, T>& measurement_variance)
{
        return make_diagonal_matrix(measurement_variance);
}

struct PositionH final
{
        template <std::size_t N, typename T>
        [[nodiscard]] Vector<N, T> operator()(const Vector<N, T>& x) const
        {
                // px = px
                return x;
        }
};

struct PositionHJ final
{
        template <std::size_t N, typename T>
        [[nodiscard]] Matrix<N, N, T> operator()(const Vector<N, T>& /*x*/) const
        {
                // px = px
                // py = py
                // Jacobian
                Matrix<N, N, T> res(0);
                for (std::size_t i = 0; i < N; ++i)
                {
                        res(i, i) = 1;
                }
                return res;
        }
};

struct PositionResidual final
{
        template <std::size_t N, typename T>
        Vector<N, T> operator()(const Vector<N, T>& a, const Vector<N, T>& b) const
        {
                return a - b;
        }
};

//

template <std::size_t N, typename T>
class Filter final : public PositionFilter<N, T>
{
        static constexpr bool LIKELIHOOD{false};

        const std::optional<T> theta_;
        const T process_variance_;
        std::optional<Ekf<N, T>> filter_;

        void reset(const Vector<N, T>& position, const Vector<N, T>& variance) override
        {
                filter_.emplace(init_x(position), init_p(variance));
        }

        void predict(const T dt) override
        {
                ASSERT(filter_);
                ASSERT(is_finite(dt));
                ASSERT(dt >= 0);

                const Matrix<N, N, T> f = f_matrix<N, T>(dt);
                filter_->predict(
                        [&](const Vector<N, T>& x)
                        {
                                return f * x;
                        },
                        [&](const Vector<N, T>& /*x*/)
                        {
                                return f;
                        },
                        q<N, T>(dt, process_variance_));
        }

        [[nodiscard]] PositionFilterUpdate<N, T> update(
                const Vector<N, T>& position,
                const Vector<N, T>& variance,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);
                ASSERT(is_finite(position));
                ASSERT(is_finite(variance));
                ASSERT(is_positive(variance));

                const Matrix<N, N, T> r = position_r(variance);

                const auto result = filter_->update(
                        PositionH(), PositionHJ(), r, position, AddX(), PositionResidual(), gate, theta_,
                        /*normalized_innovation=*/true, LIKELIHOOD);

                ASSERT(result.normalized_innovation_squared);
                return {.residual = result.residual,
                        .gate = result.gate,
                        .normalized_innovation_squared = *result.normalized_innovation_squared};
        }

        [[nodiscard]] Vector<N, T> position() const override
        {
                ASSERT(filter_);

                return filter_->x();
        }

        [[nodiscard]] Matrix<N, N, T> position_p() const override
        {
                ASSERT(filter_);

                return filter_->p();
        }

        [[nodiscard]] bool has_speed() const override
        {
                return false;
        }

        [[nodiscard]] T speed() const override
        {
                error("speed is not supported");
        }

        [[nodiscard]] T speed_p() const override
        {
                error("speed_p is not supported");
        }

        [[nodiscard]] bool has_velocity() const override
        {
                return false;
        }

        [[nodiscard]] Vector<N, T> velocity() const override
        {
                error("velocity is not supported");
        }

        [[nodiscard]] Matrix<N, N, T> velocity_p() const override
        {
                error("velocity_p is not supported");
        }

        [[nodiscard]] bool has_position_velocity_acceleration() const override
        {
                return false;
        }

        [[nodiscard]] Vector<3 * N, T> position_velocity_acceleration() const override
        {
                error("position_velocity_acceleration is not supported");
        }

        [[nodiscard]] Matrix<3 * N, 3 * N, T> position_velocity_acceleration_p() const override
        {
                error("position_velocity_acceleration_p is not supported");
        }

public:
        Filter(const T theta, const T process_variance)
                : theta_(theta),
                  process_variance_(process_variance)
        {
                ASSERT(theta_ >= 0);
                ASSERT(process_variance_ >= 0);
        }
};
}

template <std::size_t N, typename T>
std::unique_ptr<PositionFilter<N, T>> create_position_filter_lkf_0(const T theta, const T process_variance)
{
        return std::make_unique<Filter<N, T>>(theta, process_variance);
}

#define TEMPLATE_N_T(N, T) template std::unique_ptr<PositionFilter<(N), T>> create_position_filter_lkf_0<(N), T>(T, T);

#define TEMPLATE_T(T) TEMPLATE_N_T(1, T) TEMPLATE_N_T(2, T) TEMPLATE_N_T(3, T)

TEMPLATE_T(float)
TEMPLATE_T(double)
TEMPLATE_T(long double)
}
