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

#include "filter_0.h"

#include <src/com/error.h>
#include <src/com/variant.h>
#include <src/filter/core/ekf.h>
#include <src/filter/core/kinematic_models.h>
#include <src/filter/core/update_info.h>
#include <src/filter/filters/com/utility.h>
#include <src/filter/filters/noise_model.h>
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
numerical::Vector<N, T> init_x(const numerical::Vector<N, T>& position)
{
        ASSERT(is_finite(position));

        return position;
}

template <std::size_t N, typename T>
numerical::Matrix<N, N, T> init_p(const numerical::Vector<N, T>& position_variance)
{
        ASSERT(is_finite(position_variance));

        numerical::Matrix<N, N, T> res(0);
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i, i] = position_variance[i];
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
numerical::Matrix<N, N, T> f_matrix(const T /*dt*/)
{
        return block_diagonal<N>(numerical::Matrix<1, 1, T>{
                {1},
        });
}

template <std::size_t N, typename T>
numerical::Matrix<N, N, T> q(const T dt, const NoiseModel<T>& noise_model)
{
        const auto visitors = Visitors{
                [&](const ContinuousNoiseModel<T>& model)
                {
                        return block_diagonal<N>(core::continuous_white_noise<1, T>(dt, model.spectral_density));
                },
                [&](const DiscreteNoiseModel<T>& model)
                {
                        const numerical::Matrix<N, N, T> noise_transition =
                                block_diagonal<N>(numerical::Matrix<1, 1, T>{{dt}});
                        const numerical::Matrix<N, N, T> process_covariance =
                                numerical::make_diagonal_matrix(numerical::Vector<N, T>(model.variance));

                        return noise_transition * process_covariance * noise_transition.transposed();
                }};
        return std::visit(visitors, noise_model);
}

template <std::size_t N, typename T>
numerical::Matrix<N, N, T> position_r(const numerical::Vector<N, T>& measurement_variance)
{
        return numerical::make_diagonal_matrix(measurement_variance);
}

struct PositionH final
{
        template <std::size_t N, typename T>
        [[nodiscard]] numerical::Vector<N, T> operator()(const numerical::Vector<N, T>& x) const
        {
                // px = px
                return x;
        }
};

struct PositionHJ final
{
        template <std::size_t N, typename T>
        [[nodiscard]] numerical::Matrix<N, N, T> operator()(const numerical::Vector<N, T>& /*x*/) const
        {
                // px = px
                // py = py
                // Jacobian
                numerical::Matrix<N, N, T> res(0);
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i, i] = 1;
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
class FilterImpl final : public Filter0<N, T>
{
        const std::optional<T> theta_;
        std::optional<core::Ekf<N, T>> filter_;

        void reset(const numerical::Vector<N, T>& position, const numerical::Vector<N, T>& variance) override
        {
                filter_.emplace(init_x(position), init_p(variance));
        }

        void predict(const T dt, const NoiseModel<T>& noise_model, const T fading_memory_alpha) override
        {
                ASSERT(filter_);
                ASSERT(com::check_dt(dt));

                const numerical::Matrix<N, N, T> f = f_matrix<N, T>(dt);
                filter_->predict(
                        [&](const numerical::Vector<N, T>& x)
                        {
                                return f * x;
                        },
                        [&](const numerical::Vector<N, T>& /*x*/)
                        {
                                return f;
                        },
                        q<N, T>(dt, noise_model), fading_memory_alpha);
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

                return filter_->x();
        }

        [[nodiscard]] numerical::Matrix<N, N, T> position_p() const override
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
std::unique_ptr<Filter0<N, T>> create_filter_0(const T theta)
{
        return std::make_unique<FilterImpl<N, T>>(theta);
}

#define TEMPLATE(N, T) template std::unique_ptr<Filter0<(N), T>> create_filter_0<(N), T>(T);

FILTER_TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
