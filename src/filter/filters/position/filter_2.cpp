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

#include "filter_2.h"

#include "init.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/filter/core/ekf.h>
#include <src/filter/core/update_info.h>
#include <src/filter/filters/com/utility.h>
#include <src/filter/utility/instantiation.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <memory>
#include <optional>

namespace ns::filter::filters::position
{
namespace
{
constexpr bool NORMALIZED_INNOVATION{true};
constexpr bool LIKELIHOOD{false};

template <std::size_t N, typename T>
numerical::Vector<3 * N, T> init_x(const numerical::Vector<N, T>& position, const Init<T>& init)
{
        ASSERT(is_finite(position));

        numerical::Vector<3 * N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                const std::size_t b = 3 * i;
                res[b + 0] = position[i];
                res[b + 1] = init.speed;
                res[b + 2] = init.acceleration;
        }
        return res;
}

template <std::size_t N, typename T>
numerical::Matrix<3 * N, 3 * N, T> init_p(const numerical::Vector<N, T>& position_variance, const Init<T>& init)
{
        ASSERT(is_finite(position_variance));

        numerical::Matrix<3 * N, 3 * N, T> res(0);
        for (std::size_t i = 0; i < N; ++i)
        {
                const std::size_t b = 3 * i;
                res[b + 0, b + 0] = position_variance[i];
                res[b + 1, b + 1] = init.speed_variance;
                res[b + 2, b + 2] = init.acceleration_variance;
        }
        return res;
}

struct AddX final
{
        template <std::size_t N, typename T>
        [[nodiscard]] numerical::Vector<N, T> operator()(
                const numerical::Vector<N, T>& a,
                const numerical::Vector<N, T>& b) const
        {
                return a + b;
        }
};

template <std::size_t N, typename T>
numerical::Matrix<3 * N, 3 * N, T> f_matrix(const T dt)
{
        const T dt_2 = power<2>(dt) / 2;

        return block_diagonal<N>(numerical::Matrix<3, 3, T>{
                {1, dt, dt_2},
                {0,  1,   dt},
                {0,  0,    1}
        });
}

template <std::size_t N, typename T>
numerical::Matrix<3 * N, 3 * N, T> q(const T dt, const T process_variance)
{
        const T dt_2 = power<2>(dt) / 2;
        const T dt_3 = power<3>(dt) / 6;

        const numerical::Matrix<3 * N, N, T> noise_transition =
                block_diagonal<N>(numerical::Matrix<3, 1, T>{{dt_3}, {dt_2}, {dt}});
        const numerical::Matrix<N, N, T> process_covariance =
                numerical::make_diagonal_matrix(numerical::Vector<N, T>(process_variance));

        return noise_transition * process_covariance * noise_transition.transposed();
}

template <std::size_t N, typename T>
numerical::Matrix<N, N, T> position_r(const numerical::Vector<N, T>& measurement_variance)
{
        return numerical::make_diagonal_matrix(measurement_variance);
}

struct PositionH final
{
        template <std::size_t N, typename T>
        [[nodiscard]] numerical::Vector<N / 3, T> operator()(const numerical::Vector<N, T>& x) const
        {
                static_assert(N % 3 == 0);
                // px = px
                // py = py
                numerical::Vector<N / 3, T> res;
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
        [[nodiscard]] numerical::Matrix<N / 3, N, T> operator()(const numerical::Vector<N, T>& /*x*/) const
        {
                static_assert(N % 3 == 0);
                // px = px
                // py = py
                // Jacobian
                numerical::Matrix<N / 3, N, T> res(0);
                for (std::size_t i = 0; i < N / 3; ++i)
                {
                        res[i, 3 * i] = 1;
                }
                return res;
        }
};

struct PositionResidual final
{
        template <std::size_t N, typename T>
        numerical::Vector<N, T> operator()(const numerical::Vector<N, T>& a, const numerical::Vector<N, T>& b) const
        {
                return a - b;
        }
};

//

template <std::size_t N, typename T>
class FilterImpl final : public Filter2<N, T>
{
        const std::optional<T> theta_;
        const T process_variance_;
        std::optional<core::Ekf<3 * N, T>> filter_;

        void reset(
                const numerical::Vector<N, T>& position,
                const numerical::Vector<N, T>& variance,
                const Init<T>& init) override
        {
                filter_.emplace(init_x(position, init), init_p(variance, init));
        }

        void predict(const T dt) override
        {
                ASSERT(filter_);
                ASSERT(com::check_dt(dt));

                const numerical::Matrix<3 * N, 3 * N, T> f = f_matrix<N, T>(dt);
                filter_->predict(
                        [&](const numerical::Vector<3 * N, T>& x)
                        {
                                return f * x;
                        },
                        [&](const numerical::Vector<3 * N, T>& /*x*/)
                        {
                                return f;
                        },
                        q<N, T>(dt, process_variance_));
        }

        [[nodiscard]] core::UpdateInfo<N, T> update(
                const numerical::Vector<N, T>& position,
                const numerical::Vector<N, T>& variance,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);
                ASSERT(is_finite(position));
                ASSERT(com::check_variance(variance));

                const numerical::Matrix<N, N, T> r = position_r(variance);

                const core::UpdateInfo update = filter_->update(
                        PositionH(), PositionHJ(), r, position, AddX(), PositionResidual(), theta_, gate,
                        NORMALIZED_INNOVATION, LIKELIHOOD);

                return update;
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

#define TEMPLATE(N, T) template std::unique_ptr<Filter2<(N), T>> create_filter_2<(N), T>(T, T);

FILTER_TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
