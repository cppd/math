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

#pragma once

#include "constant.h"

#include <src/com/exponent.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>

namespace ns::filter::attitude::ekf
{
namespace matrix_implementation
{
// cross(a, b) = cross_matrix_1(a) * b
template <typename T>
numerical::Matrix<3, 3, T> cross_matrix_1(const numerical::Vector<3, T>& v)
{
        return {
                {    0, -v[2],  v[1]},
                { v[2],     0, -v[0]},
                {-v[1],  v[0],     0}
        };
}

template <typename T>
numerical::Matrix<3, 3, T> cross_matrix_2(const numerical::Vector<3, T>& v)
{
        const T v00 = v[0] * v[0];
        const T v01 = v[0] * v[1];
        const T v02 = v[0] * v[2];
        const T v11 = v[1] * v[1];
        const T v12 = v[1] * v[2];
        const T v22 = v[2] * v[2];
        return {
                {-v11 - v22,        v01,        v02},
                {       v01, -v00 - v22,        v12},
                {       v02,        v12, -v00 - v11}
        };
}

template <std::size_t N, typename T>
numerical::Matrix<N, N, T> add_diagonal(const T v, numerical::Matrix<N, N, T> m)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                m[i, i] = v;
        }
        return m;
}
}

template <std::size_t N, typename T>
[[nodiscard]] numerical::Matrix<3, 3, T> cross_matrix(const numerical::Vector<3, T>& v)
{
        namespace impl = matrix_implementation;

        if constexpr (N == 1)
        {
                return impl::cross_matrix_1(v);
        }
        else if constexpr (N == 2)
        {
                return impl::cross_matrix_2(v);
        }
        else if constexpr (N == 3)
        {
                return impl::cross_matrix_1(-dot(v, v) * v);
        }
        else if constexpr (N == 4)
        {
                return -dot(v, v) * impl::cross_matrix_2(v);
        }
        else if constexpr (N == 5)
        {
                return impl::cross_matrix_1(square(dot(v, v)) * v);
        }
        else if constexpr (N == 6)
        {
                return impl::cross_matrix_2(dot(v, v) * v);
        }
        else
        {
                static_assert(false);
        }
}

template <typename T>
[[nodiscard]] numerical::Matrix<3, 3, T> state_transition_matrix_3(const numerical::Vector<3, T>& w, const T dt)
{
        namespace impl = matrix_implementation;
        using Matrix = numerical::Matrix<3, 3, T>;

        const T n2 = w.norm_squared();
        const T n = std::sqrt(n2);
        const Matrix cross_1 = cross_matrix<1>(w);
        const Matrix cross_2 = cross_matrix<2>(w);

        if (n < W_THRESHOLD<T>)
        {
                const T dt2 = dt * dt;
                const T c0 = dt;
                const T c1 = dt2 / 2;
                return impl::add_diagonal(T{1}, -c0 * cross_1 + c1 * cross_2);
        }

        const T ndt = n * dt;
        const T sin = std::sin(ndt);
        const T cos = std::cos(ndt);
        const T c0 = sin / n;
        const T c1 = (1 - cos) / n2;
        return impl::add_diagonal(T{1}, -c0 * cross_1 + c1 * cross_2);
}

template <typename T>
[[nodiscard]] numerical::Matrix<6, 6, T> state_transition_matrix_6(const numerical::Vector<3, T>& w, const T dt)
{
        namespace impl = matrix_implementation;
        using Matrix = numerical::Matrix<3, 3, T>;

        const T n2 = w.norm_squared();
        const T n = std::sqrt(n2);
        const Matrix cross_1 = cross_matrix<1>(w);
        const Matrix cross_2 = cross_matrix<2>(w);

        numerical::Matrix<6, 6, T> res = numerical::IDENTITY_MATRIX<6, T>;

        if (n < W_THRESHOLD<T>)
        {
                const T dt2 = dt * dt;
                const T dt3 = dt2 * dt;
                const T c0 = dt;
                const T c1 = dt2 / 2;
                const T c2 = dt3 / 6;
                const Matrix theta = impl::add_diagonal(T{1}, -c0 * cross_1 + c1 * cross_2);
                const Matrix psi = impl::add_diagonal(-dt, c1 * cross_1 - c2 * cross_2);
                numerical::set_block<0, 0>(res, theta);
                numerical::set_block<0, 3>(res, psi);
                return res;
        }

        const T ndt = n * dt;
        const T n3 = n2 * n;
        const T sin = std::sin(ndt);
        const T cos = std::cos(ndt);
        const T c0 = sin / n;
        const T c1 = (1 - cos) / n2;
        const T c2 = (ndt - sin) / n3;
        const Matrix theta = impl::add_diagonal(T{1}, -c0 * cross_1 + c1 * cross_2);
        const Matrix psi = impl::add_diagonal(-dt, c1 * cross_1 - c2 * cross_2);
        numerical::set_block<0, 0>(res, theta);
        numerical::set_block<0, 3>(res, psi);
        return res;
}

template <typename T>
[[nodiscard]] numerical::Matrix<3, 3, T> noise_covariance_matrix_3(const T vr, const T dt)
{
        return numerical::make_diagonal_matrix<3, T>(vr * dt);
}

template <typename T>
[[nodiscard]] numerical::Matrix<6, 6, T> noise_covariance_matrix_6(
        const numerical::Vector<3, T>& w,
        const T vr,
        const T vw,
        const T dt)
{
        namespace impl = matrix_implementation;
        using Matrix = numerical::Matrix<3, 3, T>;

        const T n2 = w.norm_squared();
        const T n = std::sqrt(n2);
        const Matrix cross_1 = cross_matrix<1>(w);
        const Matrix cross_2 = cross_matrix<2>(w);

        Matrix q11;
        Matrix q12;

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
                q11 = impl::add_diagonal(vr * dt, vw * impl::add_diagonal(c0, c1 * cross_2));
                q12 = -vw * impl::add_diagonal(c2, -c3 * cross_1 + c4 * cross_2);
        }
        else
        {
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
                q11 = impl::add_diagonal(vr * dt, vw * impl::add_diagonal(c0, c1 * cross_2));
                q12 = -vw * impl::add_diagonal(c2, -c3 * cross_1 + c4 * cross_2);
        }

        numerical::Matrix<6, 6, T> res;
        numerical::set_block<0, 0>(res, q11);
        numerical::set_block<0, 3>(res, q12);
        numerical::set_block<3, 0>(res, q12.transposed());
        numerical::set_block<3, 3>(res, numerical::make_diagonal_matrix<3, T>(vw * dt));
        return res;
}
}
