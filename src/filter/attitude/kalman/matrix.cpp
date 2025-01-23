/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "matrix.h"

#include "constant.h"
#include "cross_matrix.h"

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

namespace ns::filter::attitude::kalman
{
template <typename T>
numerical::Matrix<3, 3, T> ekf_state_transition_matrix_3(const numerical::Vector<3, T>& w, const T dt)
{
        const auto make = [&](const T c0, const T c1)
        {
                using Matrix = numerical::Matrix<3, 3, T>;

                const Matrix cross_1 = cross_matrix<1>(w);
                const Matrix cross_2 = cross_matrix<2>(w);

                return add_diagonal(T{1}, -c0 * cross_1 + c1 * cross_2);
        };

        const T n2 = w.norm_squared();
        const T n = std::sqrt(n2);

        if (n < W_THRESHOLD<T>)
        {
                const T dt2 = dt * dt;

                const T c0 = dt;
                const T c1 = dt2 / 2;

                return make(c0, c1);
        }

        const T ndt = n * dt;
        const T sin = std::sin(ndt);
        const T cos = std::cos(ndt);

        const T c0 = sin / n;
        const T c1 = (1 - cos) / n2;

        return make(c0, c1);
}

template <typename T>
numerical::Matrix<6, 6, T> ekf_state_transition_matrix_6(const numerical::Vector<3, T>& w, const T dt)
{
        const auto make = [&](const T c0, const T c1, const T c2)
        {
                using Matrix = numerical::Matrix<3, 3, T>;

                const Matrix cross_1 = cross_matrix<1>(w);
                const Matrix cross_2 = cross_matrix<2>(w);

                const Matrix theta = add_diagonal(T{1}, -c0 * cross_1 + c1 * cross_2);
                const Matrix psi = add_diagonal(-dt, c1 * cross_1 - c2 * cross_2);

                numerical::Matrix<6, 6, T> res = numerical::IDENTITY_MATRIX<6, T>;

                numerical::set_block<0, 0>(res, theta);
                numerical::set_block<0, 3>(res, psi);

                return res;
        };

        const T n2 = w.norm_squared();
        const T n = std::sqrt(n2);

        if (n < W_THRESHOLD<T>)
        {
                const T dt2 = dt * dt;
                const T dt3 = dt2 * dt;

                const T c0 = dt;
                const T c1 = dt2 / 2;
                const T c2 = dt3 / 6;

                return make(c0, c1, c2);
        }

        const T ndt = n * dt;
        const T n3 = n2 * n;
        const T sin = std::sin(ndt);
        const T cos = std::cos(ndt);

        const T c0 = sin / n;
        const T c1 = (1 - cos) / n2;
        const T c2 = (ndt - sin) / n3;

        return make(c0, c1, c2);
}

template <typename T>
numerical::Matrix<3, 3, T> noise_covariance_matrix_3(const T vr, const T dt)
{
        return numerical::make_diagonal_matrix<3, T>(vr * dt);
}

template <typename T>
numerical::Matrix<6, 6, T> noise_covariance_matrix_6(
        const numerical::Vector<3, T>& w,
        const T vr,
        const T vw,
        const T dt)
{
        const auto make = [&](const T c0, const T c1, const T c2, const T c3, const T c4)
        {
                using Matrix = numerical::Matrix<3, 3, T>;

                const Matrix cross_1 = cross_matrix<1>(w);
                const Matrix cross_2 = cross_matrix<2>(w);

                const Matrix q11 = add_diagonal(vr * dt, vw * add_diagonal(c0, c1 * cross_2));
                const Matrix q12 = -vw * add_diagonal(c2, -c3 * cross_1 + c4 * cross_2);

                numerical::Matrix<6, 6, T> res;

                numerical::set_block<0, 0>(res, q11);
                numerical::set_block<0, 3>(res, q12);
                numerical::set_block<3, 0>(res, q12.transposed());
                numerical::set_block<3, 3>(res, numerical::make_diagonal_matrix<3, T>(vw * dt));

                return res;
        };

        const T n2 = w.norm_squared();
        const T n = std::sqrt(n2);

        if (n < W_THRESHOLD<T>)
        {
                const T dt2 = dt * dt;
                const T dt3 = dt2 * dt;
                const T dt4 = dt3 * dt;
                const T dt5 = dt4 * dt;

                const T c0 = dt3 / 3;
                const T c1 = dt5 / (5 * 4 * 3);
                const T c2 = dt2 / 2;
                const T c3 = dt3 / (3 * 2);
                const T c4 = dt4 / (4 * 3 * 2);

                return make(c0, c1, c2, c3, c4);
        }

        const T n3 = n2 * n;
        const T n4 = n3 * n;
        const T n5 = n4 * n;
        const T ndt = n * dt;
        const T ndt2 = ndt * ndt;
        const T ndt3 = ndt2 * ndt;
        const T sin = std::sin(ndt);
        const T cos = std::cos(ndt);
        const T dt2 = dt * dt;
        const T dt3 = dt2 * dt;

        const T c0 = dt3 / 3;
        const T c1 = (ndt3 / 3 + 2 * sin - 2 * ndt) / n5;
        const T c2 = dt2 / 2;
        const T c3 = (ndt - sin) / n3;
        const T c4 = (ndt2 / 2 + cos - 1) / n4;

        return make(c0, c1, c2, c3, c4);
}

#define TEMPLATE(T)                                                                                           \
        template numerical::Matrix<3, 3, T> ekf_state_transition_matrix_3(const numerical::Vector<3, T>&, T); \
        template numerical::Matrix<6, 6, T> ekf_state_transition_matrix_6(const numerical::Vector<3, T>&, T); \
        template numerical::Matrix<3, 3, T> noise_covariance_matrix_3(T, T);                                  \
        template numerical::Matrix<6, 6, T> noise_covariance_matrix_6(const numerical::Vector<3, T>&, T, T, T);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
