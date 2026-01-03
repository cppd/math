/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cmath>
#include <cstddef>

namespace ns::filter::filters::com
{
template <std::size_t N, typename T>
[[nodiscard]] bool check_variance(const numerical::Vector<N, T>& v)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!(std::isfinite(v[i]) && v[i] > 0))
                {
                        return false;
                }
        }
        return true;
}

template <typename T>
[[nodiscard]] numerical::Vector<1, T> angle_variance(
        const numerical::Vector<1, T>& /*velocity*/,
        const numerical::Matrix<1, 1, T>& /*velocity_p*/)
{
        return numerical::Vector<1, T>(0);
}

template <typename T>
[[nodiscard]] numerical::Vector<2, T> angle_variance(
        const numerical::Vector<2, T>& velocity,
        const numerical::Matrix<2, 2, T>& velocity_p)
{
        // angle = atan(y/x)
        // Jacobian
        //  -y/(x*x+y*y) x/(x*x+y*y)
        const T ns = velocity.norm_squared();
        const T x = velocity[0];
        const T y = velocity[1];
        const numerical::Matrix<1, 2, T> error_propagation{
                {-y / ns, x / ns}
        };
        const numerical::Matrix<1, 1, T> p = error_propagation * velocity_p * error_propagation.transposed();
        const T r = p[0, 0];
        return numerical::Vector<2, T>(r, r);
}

template <std::size_t N, typename T>
        requires (N >= 3)
[[nodiscard]] numerical::Vector<N, T> angle_variance(
        const numerical::Vector<N, T>& velocity,
        const numerical::Matrix<N, N, T>& velocity_p)
{
        // angle0 = acos(x0/sqrt(x0*x0+x1*x1+x2*x2+...))
        // angle1 = acos(x1/sqrt(x0*x0+x1*x1+x2*x2+...))
        // angle2 = acos(x2/sqrt(x0*x0+x1*x1+x2*x2+...))
        //
        // Jacobian
        // a0=ArcCos[x0/Sqrt[x0*x0+x1*x1+x2*x2]];
        // a1=ArcCos[x1/Sqrt[x0*x0+x1*x1+x2*x2]];
        // a2=ArcCos[x2/Sqrt[x0*x0+x1*x1+x2*x2]];
        // Assuming[x0>0&&x1>0&&x2>0,Simplify[D[{a0,a1,a2},{{x0,x1,x2}}]]]
        //
        // -(Sqrt[x1^2 + x2^2]/(x0^2 + x1^2 + x2^2)),
        // (x0*x1)/(Sqrt[x1^2 + x2^2]*(x0^2 + x1^2 + x2^2)),
        // (x0*x2)/(Sqrt[x1^2 + x2^2]*(x0^2 + x1^2 + x2^2))

        const auto norm_exclude_i = [&](const std::size_t i)
        {
                numerical::Vector<N - 1, T> v;
                std::size_t n = 0;
                for (std::size_t j = 0; j < N; ++j)
                {
                        if (i != j)
                        {
                                v[n++] = velocity[j];
                        }
                }
                return v.norm();
        };

        const T norm_squared = velocity.norm_squared();

        numerical::Vector<N, T> res;

        for (std::size_t i = 0; i < N; ++i)
        {
                const T norm_i = norm_exclude_i(i);
                const T denominator = norm_i * norm_squared;

                numerical::Matrix<1, N, T> error_propagation;
                error_propagation[0, i] = -norm_i / norm_squared;
                for (std::size_t j = 0; j < N; ++j)
                {
                        if (i != j)
                        {
                                error_propagation[0, j] = velocity[i] * velocity[j] / denominator;
                        }
                }
                const numerical::Matrix<1, 1, T> p = error_propagation * velocity_p * error_propagation.transposed();
                res[i] = p[0, 0];
        }

        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] T speed_variance(const numerical::Vector<N, T>& velocity, const numerical::Matrix<N, N, T>& velocity_p)
{
        // speed = sqrt(vx*vx + vy*vy)
        // Jacobian
        //  x/sqrt(x*x+y*y) y/sqrt(x*x+y*y)
        const numerical::Matrix<1, N, T> error_propagation(velocity.normalized());
        const numerical::Matrix<1, 1, T> p = error_propagation * velocity_p * error_propagation.transposed();
        return p[0, 0];
}
}
