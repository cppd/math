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

#include "position_filter_lkf_1.h"

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
struct Init final
{
        static constexpr Vector<N, T> VELOCITY{0};
        static constexpr Vector<N, T> VELOCITY_VARIANCE{square<T>(30)};
};

template <std::size_t N, typename T>
Vector<2 * N, T> init_x(const Vector<N, T>& position)
{
        ASSERT(is_finite(position));

        Vector<2 * N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                const std::size_t b = 2 * i;
                res[b + 0] = position[i];
                res[b + 1] = Init<N, T>::VELOCITY[i];
        }
        return res;
}

template <std::size_t N, typename T>
Matrix<2 * N, 2 * N, T> init_p(const Vector<N, T>& position_variance)
{
        ASSERT(is_finite(position_variance));

        Matrix<2 * N, 2 * N, T> res(0);
        for (std::size_t i = 0; i < N; ++i)
        {
                const std::size_t b = 2 * i;
                res(b + 0, b + 0) = position_variance[i];
                res(b + 1, b + 1) = Init<N, T>::VELOCITY_VARIANCE[i];
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
Matrix<2 * N, 2 * N, T> f_matrix(const T dt)
{
        return block_diagonal<N>(Matrix<2, 2, T>{
                {1, dt},
                {0,  1}
        });
}

template <std::size_t N, typename T>
Matrix<2 * N, 2 * N, T> q(const T dt, const T process_variance)
{
        const T dt_2 = power<2>(dt) / 2;

        const Matrix<2 * N, N, T> noise_transition = block_diagonal<N>(Matrix<2, 1, T>{{dt_2}, {dt}});
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
        [[nodiscard]] Vector<N / 2, T> operator()(const Vector<N, T>& x) const
        {
                static_assert(N % 2 == 0);
                // px = px
                // py = py
                Vector<N / 2, T> res;
                for (std::size_t i = 0; i < N / 2; ++i)
                {
                        res[i] = x[2 * i];
                }
                return res;
        }
};

struct PositionHJ final
{
        template <std::size_t N, typename T>
        [[nodiscard]] Matrix<N / 2, N, T> operator()(const Vector<N, T>& /*x*/) const
        {
                static_assert(N % 2 == 0);
                // px = px
                // py = py
                // Jacobian
                Matrix<N / 2, N, T> res(0);
                for (std::size_t i = 0; i < N / 2; ++i)
                {
                        res(i, 2 * i) = 1;
                }
                return res;
        }
};

template <std::size_t N, typename T>
Vector<N, T> position_residual(const Vector<N, T>& a, const Vector<N, T>& b)
{
        return a - b;
}

//

template <std::size_t N, typename T>
class Filter final : public PositionFilter<N, T>
{
        static constexpr bool LIKELIHOOD{false};

        const std::optional<T> theta_;
        const T process_variance_;
        std::optional<Ekf<2 * N, T>> filter_;

        void reset(const Vector<N, T>& position, const Vector<N, T>& variance) override
        {
                filter_.emplace(init_x(position), init_p(variance));
        }

        void predict(const T dt) override
        {
                ASSERT(filter_);
                ASSERT(is_finite(dt));
                ASSERT(dt >= 0);

                const Matrix<2 * N, 2 * N, T> f = f_matrix<N, T>(dt);
                filter_->predict(
                        [&](const Vector<2 * N, T>& x)
                        {
                                return f * x;
                        },
                        [&](const Vector<2 * N, T>& /*x*/)
                        {
                                return f;
                        },
                        q<N, T>(dt, process_variance_));
        }

        [[nodiscard]] std::optional<PositionFilterUpdate<N, T>> update(
                const Vector<N, T>& position,
                const Vector<N, T>& variance,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);
                ASSERT(is_finite(position));
                ASSERT(is_finite(variance));
                ASSERT(is_positive(variance));

                const Matrix<N, N, T> r = position_r(variance);

                Vector<N, T> residual;

                const auto f_residual = [&](const Vector<N, T>& a, const Vector<N, T>& b)
                {
                        residual = position_residual(a, b);
                        return residual;
                };

                if (!filter_->update(
                                    PositionH(), PositionHJ(), r, position, AddX(), f_residual, gate, theta_,
                                    LIKELIHOOD)
                             .gate)
                {
                        return {
                                {.r = r, .residual = residual}
                        };
                }

                ASSERT(gate);
                return {};
        }

        [[nodiscard]] Vector<N, T> position() const override
        {
                ASSERT(filter_);

                return slice<0, 2>(filter_->x());
        }

        [[nodiscard]] Matrix<N, N, T> position_p() const override
        {
                ASSERT(filter_);

                return slice<0, 2>(filter_->p());
        }

        [[nodiscard]] bool has_speed() const override
        {
                return true;
        }

        [[nodiscard]] T speed() const override
        {
                return velocity().norm();
        }

        [[nodiscard]] T speed_p() const override
        {
                return compute_speed_p(velocity(), velocity_p());
        }

        [[nodiscard]] bool has_velocity() const override
        {
                return true;
        }

        [[nodiscard]] Vector<N, T> velocity() const override
        {
                ASSERT(filter_);

                return slice<1, 2>(filter_->x());
        }

        [[nodiscard]] Matrix<N, N, T> velocity_p() const override
        {
                ASSERT(filter_);

                return slice<1, 2>(filter_->p());
        }

        [[nodiscard]] bool has_position_velocity_acceleration() const override
        {
                return false;
        }

        [[nodiscard]] Vector<3 * N, T> position_velocity_acceleration() const override
        {
                error("no position_velocity_acceleration is not supported");
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
std::unique_ptr<PositionFilter<N, T>> create_position_filter_lkf_1(const T theta, const T process_variance)
{
        return std::make_unique<Filter<N, T>>(theta, process_variance);
}

#define TEMPLATE_N_T(N, T) template std::unique_ptr<PositionFilter<(N), T>> create_position_filter_lkf_1<(N), T>(T, T);

#define TEMPLATE_T(T) TEMPLATE_N_T(1, T) TEMPLATE_N_T(2, T) TEMPLATE_N_T(3, T)

TEMPLATE_T(float)
TEMPLATE_T(double)
TEMPLATE_T(long double)
}
