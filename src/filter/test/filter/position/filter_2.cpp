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

#include "filter_2.h"

#include "../../../ekf.h"
#include "../../utility/utility.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <optional>

namespace ns::filter::test::filter::position
{
namespace
{
template <std::size_t N, typename T>
struct Init final
{
        static constexpr Vector<N, T> VELOCITY{0};
        static constexpr Vector<N, T> VELOCITY_VARIANCE{square<T>(30)};

        static constexpr Vector<N, T> ACCELERATION{0};
        static constexpr Vector<N, T> ACCELERATION_VARIANCE{square<T>(10)};
};

template <std::size_t N, typename T>
Vector<3 * N, T> init_x(const Vector<N, T>& position)
{
        ASSERT(is_finite(position));

        Vector<3 * N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                const std::size_t b = 3 * i;
                res[b + 0] = position[i];
                res[b + 1] = Init<N, T>::VELOCITY[i];
                res[b + 2] = Init<N, T>::ACCELERATION[i];
        }
        return res;
}

template <std::size_t N, typename T>
Matrix<3 * N, 3 * N, T> init_p(const Vector<N, T>& position_variance)
{
        ASSERT(is_finite(position_variance));

        Matrix<3 * N, 3 * N, T> res(0);
        for (std::size_t i = 0; i < N; ++i)
        {
                const std::size_t b = 3 * i;
                res(b + 0, b + 0) = position_variance[i];
                res(b + 1, b + 1) = Init<N, T>::VELOCITY_VARIANCE[i];
                res(b + 2, b + 2) = Init<N, T>::ACCELERATION_VARIANCE[i];
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
Matrix<3 * N, 3 * N, T> f_matrix(const T dt)
{
        const T dt_2 = power<2>(dt) / 2;

        return block_diagonal<N>(Matrix<3, 3, T>{
                {1, dt, dt_2},
                {0,  1,   dt},
                {0,  0,    1}
        });
}

template <std::size_t N, typename T>
Matrix<3 * N, 3 * N, T> q(const T dt, const T process_variance)
{
        const T dt_2 = power<2>(dt) / 2;
        const T dt_3 = power<3>(dt) / 6;

        const Matrix<3 * N, N, T> noise_transition = block_diagonal<N>(Matrix<3, 1, T>{{dt_3}, {dt_2}, {dt}});
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
        [[nodiscard]] Vector<N / 3, T> operator()(const Vector<N, T>& x) const
        {
                static_assert(N % 3 == 0);
                // px = px
                // py = py
                Vector<N / 3, T> res;
                for (std::size_t i = 0; i < N / 3; ++i)
                {
                        res[i] = x[3 * i];
                }
                return res;
        }
};

struct PositionHJ final
{
        template <std::size_t N, typename T>
        [[nodiscard]] Matrix<N / 3, N, T> operator()(const Vector<N, T>& /*x*/) const
        {
                static_assert(N % 3 == 0);
                // px = px
                // py = py
                // Jacobian
                Matrix<N / 3, N, T> res(0);
                for (std::size_t i = 0; i < N / 3; ++i)
                {
                        res(i, 3 * i) = 1;
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
class FilterImpl final : public Filter2<N, T>
{
        static constexpr bool LIKELIHOOD{false};

        const std::optional<T> theta_;
        const T process_variance_;
        std::optional<Ekf<3 * N, T>> filter_;

        void reset(const Vector<N, T>& position, const Vector<N, T>& variance) override
        {
                filter_.emplace(init_x(position), init_p(variance));
        }

        void predict(const T dt) override
        {
                ASSERT(filter_);
                ASSERT(is_finite(dt));
                ASSERT(dt >= 0);

                const Matrix<3 * N, 3 * N, T> f = f_matrix<N, T>(dt);
                filter_->predict(
                        [&](const Vector<3 * N, T>& x)
                        {
                                return f * x;
                        },
                        [&](const Vector<3 * N, T>& /*x*/)
                        {
                                return f;
                        },
                        q<N, T>(dt, process_variance_));
        }

        [[nodiscard]] Filter2<N, T>::Update update(
                const Vector<N, T>& position,
                const Vector<N, T>& variance,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);
                ASSERT(is_finite(position));
                ASSERT(is_finite(variance));
                ASSERT(utility::is_positive(variance));

                const Matrix<N, N, T> r = position_r(variance);

                const UpdateInfo update = filter_->update(
                        PositionH(), PositionHJ(), r, position, AddX(), PositionResidual(), theta_, gate,
                        /*normalized_innovation=*/true, LIKELIHOOD);

                ASSERT(update.normalized_innovation_squared);
                return {.residual = update.residual,
                        .gate = update.gate,
                        .normalized_innovation_squared = *update.normalized_innovation_squared};
        }

        [[nodiscard]] Vector<N, T> position() const override
        {
                ASSERT(filter_);

                return slice<0, 3>(filter_->x());
        }

        [[nodiscard]] Matrix<N, N, T> position_p() const override
        {
                ASSERT(filter_);

                return slice<0, 3>(filter_->p());
        }

        [[nodiscard]] T speed() const override
        {
                return velocity().norm();
        }

        [[nodiscard]] T speed_p() const override
        {
                return utility::compute_speed_p(velocity(), velocity_p());
        }

        [[nodiscard]] Vector<N, T> velocity() const override
        {
                ASSERT(filter_);

                return slice<1, 3>(filter_->x());
        }

        [[nodiscard]] Matrix<N, N, T> velocity_p() const override
        {
                ASSERT(filter_);

                return slice<1, 3>(filter_->p());
        }

        [[nodiscard]] Vector<2 * N, T> position_velocity() const override
        {
                ASSERT(filter_);

                const Vector<3 * N, T>& x = filter_->x();
                Vector<2 * N, T> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        for (std::size_t j = 0; j < 2; ++j)
                        {
                                res[2 * i + j] = x[3 * i + j];
                        }
                }
                return res;
        }

        [[nodiscard]] Matrix<2 * N, 2 * N, T> position_velocity_p() const override
        {
                ASSERT(filter_);

                const Matrix<3 * N, 3 * N, T>& p = filter_->p();
                Matrix<2 * N, 2 * N, T> res;
                for (std::size_t r = 0; r < N; ++r)
                {
                        for (std::size_t i = 0; i < 2; ++i)
                        {
                                for (std::size_t c = 0; c < N; ++c)
                                {
                                        for (std::size_t j = 0; j < 2; ++j)
                                        {
                                                res(2 * r + i, 2 * c + j) = p(3 * r + i, 3 * c + j);
                                        }
                                }
                        }
                }
                return res;
        }

        [[nodiscard]] Vector<3 * N, T> position_velocity_acceleration() const override
        {
                ASSERT(filter_);

                return filter_->x();
        }

        [[nodiscard]] Matrix<3 * N, 3 * N, T> position_velocity_acceleration_p() const override
        {
                ASSERT(filter_);

                return filter_->p();
        }

public:
        FilterImpl(const T theta, const T process_variance)
                : theta_(theta),
                  process_variance_(process_variance)
        {
                ASSERT(theta_ >= 0);
                ASSERT(process_variance_ >= 0);
        }
};
}

template <std::size_t N, typename T>
std::unique_ptr<Filter2<N, T>> create_filter_2(const T theta, const T process_variance)
{
        return std::make_unique<FilterImpl<N, T>>(theta, process_variance);
}

#define TEMPLATE_N_T(N, T) template std::unique_ptr<Filter2<(N), T>> create_filter_2<(N), T>(T, T);

#define TEMPLATE_T(T) TEMPLATE_N_T(1, T) TEMPLATE_N_T(2, T) TEMPLATE_N_T(3, T)

TEMPLATE_T(float)
TEMPLATE_T(double)
TEMPLATE_T(long double)
}
