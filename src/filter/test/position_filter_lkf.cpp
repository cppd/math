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

#include "position_filter_lkf.h"

#include "utility.h"

#include "../consistency.h"
#include "../ekf.h"

#include <src/com/error.h>
#include <src/com/exponent.h>

namespace ns::filter::test
{
namespace
{
template <typename T>
struct Init final
{
        static constexpr Vector<2, T> VELOCITY{0};
        static constexpr Vector<2, T> ACCELERAION{0};
        static constexpr T SPEED_VARIANCE{square<T>(30)};
        static constexpr T ACCELERATION_VARIANCE{square<T>(10)};
};

template <typename T>
Vector<6, T> x(const Vector<2, T>& position)
{
        ASSERT(is_finite(position));

        return {position[0], Init<T>::VELOCITY[0], Init<T>::ACCELERAION[0],
                position[1], Init<T>::VELOCITY[1], Init<T>::ACCELERAION[1]};
}

template <typename T>
Matrix<6, 6, T> p(const T position_variance)
{
        ASSERT(is_finite(position_variance));

        const T pv = position_variance;
        const T sv = Init<T>::SPEED_VARIANCE;
        const T av = Init<T>::ACCELERATION_VARIANCE;
        return make_diagonal_matrix<6, T>({pv, sv, av, pv, sv, av});
}

template <typename T>
Vector<6, T> add_x(const Vector<6, T>& a, const Vector<6, T>& b)
{
        return a + b;
}

template <typename T>
Matrix<6, 6, T> f(const T dt)
{
        const T dt_2 = square(dt) / 2;
        return {
                {1, dt, dt_2, 0,  0,    0},
                {0,  1,   dt, 0,  0,    0},
                {0,  0,    1, 0,  0,    0},
                {0,  0,    0, 1, dt, dt_2},
                {0,  0,    0, 0,  1,   dt},
                {0,  0,    0, 0,  0,    1}
        };
}

template <typename T>
Matrix<6, 6, T> q(const T dt, const T process_variance)
{
        const T dt_2 = power<2>(dt) / 2;
        const T dt_3 = power<3>(dt) / 6;
        const Matrix<6, 2, T> noise_transition{
                {dt_3,    0},
                {dt_2,    0},
                {  dt,    0},
                {   0, dt_3},
                {   0, dt_2},
                {   0,   dt},
        };

        const T p = process_variance;
        const Matrix<2, 2, T> process_covariance{
                {p, 0},
                {0, p}
        };

        return noise_transition * process_covariance * noise_transition.transposed();
}

template <typename T>
Matrix<2, 2, T> position_r(const T measurement_variance)
{
        const T mv = measurement_variance;
        return {
                {mv,  0},
                { 0, mv}
        };
}

template <typename T>
Vector<2, T> position_h(const Vector<6, T>& x)
{
        // px = px
        // py = py
        return {x[0], x[3]};
}

template <typename T>
Matrix<2, 6, T> position_hj(const Vector<6, T>& /*x*/)
{
        // px = px
        // py = py
        // Jacobian
        return {
                {1, 0, 0, 0, 0, 0},
                {0, 0, 0, 1, 0, 0}
        };
}

template <typename T>
Vector<2, T> position_residual(const Vector<2, T>& a, const Vector<2, T>& b)
{
        return a - b;
}

//

template <typename T>
class Filter final : public PositionFilter<T>
{
        Ekf<6, T> filter_;
        NormalizedSquared<2, T> nis_;
        T theta_;
        T process_variance_;

        void predict(const T dt) override
        {
                ASSERT(is_finite(dt));
                ASSERT(dt >= 0);

                const Matrix<6, 6, T> f_matrix = f(dt);
                filter_.predict(
                        [&](const Vector<6, T>& x)
                        {
                                return f_matrix * x;
                        },
                        [&](const Vector<6, T>& /*x*/)
                        {
                                return f_matrix;
                        },
                        q(dt, process_variance_));
        }

        void update(const Vector<2, T>& position, const T position_variance) override
        {
                ASSERT(is_finite(position));
                ASSERT(is_finite(position_variance));
                ASSERT(position_variance >= 0);

                const Matrix<2, 2, T> r = position_r(position_variance);
                filter_.update(
                        position_h<T>, position_hj<T>, r, position, add_x<T>,
                        [&](const Vector<2, T>& a, const Vector<2, T>& b)
                        {
                                const auto residual = position_residual<T>(a, b);
                                nis_.add(residual, r);
                                return residual;
                        },
                        theta_);
        }

        [[nodiscard]] Vector<2, T> position() const override
        {
                return {filter_.x()[0], filter_.x()[3]};
        }

        [[nodiscard]] Matrix<2, 2, T> position_p() const override
        {
                return {
                        {filter_.p()(0, 0), filter_.p()(0, 3)},
                        {filter_.p()(3, 0), filter_.p()(3, 3)}
                };
        }

        [[nodiscard]] T speed() const override
        {
                return velocity().norm();
        }

        [[nodiscard]] T speed_p() const override
        {
                return compute_speed_p(velocity(), velocity_p());
        }

        [[nodiscard]] Vector<2, T> velocity() const override
        {
                return {filter_.x()[1], filter_.x()[4]};
        }

        [[nodiscard]] Matrix<2, 2, T> velocity_p() const override
        {
                return {
                        {filter_.p()(1, 1), filter_.p()(1, 4)},
                        {filter_.p()(4, 1), filter_.p()(4, 4)}
                };
        }

        [[nodiscard]] T angle() const override
        {
                return std::atan2(filter_.x()[4], filter_.x()[1]);
        }

        [[nodiscard]] T angle_p() const override
        {
                return compute_angle_p(velocity(), velocity_p());
        }

        [[nodiscard]] std::string nis_position_check_string() const override
        {
                return nis_.check_string();
        }

public:
        Filter(const Vector<2, T>& position, const T position_variance, const T theta, const T process_variance)
                : filter_(x(position), p(position_variance)),
                  theta_(theta),
                  process_variance_(process_variance)
        {
                ASSERT(process_variance >= 0);
        }
};
}

template <typename T>
std::unique_ptr<PositionFilter<T>> create_position_filter_lkf(
        const Vector<2, T>& position,
        const T position_variance,
        const T theta,
        const T process_variance)
{
        return std::make_unique<Filter<T>>(position, position_variance, theta, process_variance);
}

#define TEMPLATE(T) \
        template std::unique_ptr<PositionFilter<T>> create_position_filter_lkf(const Vector<2, T>&, T, T, T);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
