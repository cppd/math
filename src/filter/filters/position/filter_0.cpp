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
#include <src/filter/ekf.h>
#include <src/filter/filters/utility/utility.h>
#include <src/filter/settings/instantiation.h>
#include <src/filter/update_info.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <memory>
#include <optional>

namespace ns::filter::filters::position
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
                res[i, i] = position_variance[i];
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
                        res[i, i] = 1;
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
class FilterImpl final : public Filter0<N, T>
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
                ASSERT(utility::check_dt(dt));

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

        [[nodiscard]] Filter0<N, T>::Update update(
                const Vector<N, T>& position,
                const Vector<N, T>& variance,
                const std::optional<T> gate) override
        {
                ASSERT(filter_);
                ASSERT(is_finite(position));
                ASSERT(utility::check_variance(variance));

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

                return filter_->x();
        }

        [[nodiscard]] Matrix<N, N, T> position_p() const override
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
std::unique_ptr<Filter0<N, T>> create_filter_0(const T theta, const T process_variance)
{
        return std::make_unique<FilterImpl<N, T>>(theta, process_variance);
}

#define TEMPLATE(N, T) template std::unique_ptr<Filter0<(N), T>> create_filter_0<(N), T>(T, T);

FILTER_TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
