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

#include "ekf.h"

#include <src/com/error.h>
#include <src/filter/core/ekf.h>
#include <src/filter/core/models.h>
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

template <typename T, bool INF>
class Filter final : public TestEkf<T, INF>
{
        static constexpr std::optional<T> GATE{};
        static constexpr bool NORMALIZED_INNOVATION{true};
        static constexpr bool LIKELIHOOD{true};
        static constexpr std::optional<T> THETA{INF ? 0.01L : std::optional<T>()};

        std::optional<Ekf<2, T>> filter_;

        void reset(const numerical::Vector<2, T>& x, const numerical::Matrix<2, 2, T>& p) override
        {
                filter_.emplace(x, p);
        }

        void predict(const T dt, const T process_variance) override
        {
                ASSERT(filter_);

                const numerical::Matrix<2, 2, T> q = discrete_white_noise<2, T>(dt, process_variance);

                // x[0] = x[0] + dt * x[1]
                // x[1] = x[1]
                // Jacobian matrix
                //  1 dt
                //  0  1
                const numerical::Matrix<2, 2, T> f_matrix{
                        {1, dt},
                        {0,  1}
                };

                filter_->predict(
                        [&](const numerical::Vector<2, T>& x)
                        {
                                return f_matrix * x;
                        },
                        [&](const numerical::Vector<2, T>& /*x*/)
                        {
                                return f_matrix;
                        },
                        q);
        }

        void update_position(const T position, const T position_variance) override
        {
                ASSERT(filter_);

                const numerical::Matrix<1, 1, T> r{{position_variance}};

                // x = x[0]
                // Jacobian matrix
                //  1 0
                const auto h = [](const numerical::Vector<2, T>& x)
                {
                        return numerical::Vector<1, T>(x[0]);
                };
                const auto h_jacobian = [](const numerical::Vector<2, T>& /*x*/)
                {
                        return numerical::Matrix<1, 2, T>{
                                {1, 0}
                        };
                };

                filter_->update(
                        h, h_jacobian, r, numerical::Vector<1, T>(position), Add(), Residual(), THETA, GATE,
                        NORMALIZED_INNOVATION, LIKELIHOOD);
        }

        void update_position_speed(const T position, const T position_variance, const T speed, const T speed_variance)
                override
        {
                ASSERT(filter_);

                const numerical::Matrix<2, 2, T> r{
                        {position_variance,              0},
                        {                0, speed_variance}
                };

                // x = x[0]
                // v = x[1]
                // Jacobian matrix
                //  1 0
                //  0 1
                const auto h = [](const numerical::Vector<2, T>& x)
                {
                        return numerical::Vector<2, T>(x[0], x[1]);
                };
                const auto h_jacobian = [](const numerical::Vector<2, T>& /*x*/)
                {
                        return numerical::Matrix<2, 2, T>{
                                {1, 0},
                                {0, 1}
                        };
                };

                filter_->update(
                        h, h_jacobian, r, numerical::Vector<2, T>(position, speed), Add(), Residual(), THETA, GATE,
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

        [[nodiscard]] std::string name() const override
        {
                return INF ? "EXTENDED_H_INFINITY" : "EKF";
        }

public:
        Filter() = default;
};
}

template <typename T, bool INF>
[[nodiscard]] std::unique_ptr<TestEkf<T, INF>> create_test_ekf()
{
        return std::make_unique<Filter<T, INF>>();
}

#define INSTANTIATION(T)                                               \
        template std::unique_ptr<TestEkf<T, false>> create_test_ekf(); \
        template std::unique_ptr<TestEkf<T, true>> create_test_ekf();

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
