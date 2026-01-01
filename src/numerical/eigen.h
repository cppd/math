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

/*
Jaan Kiusalaas.
Numerical Methods in Engineering with Python 3.
Cambridge University Press, 2013.

9 Symmetric Matrix Eigenvalue Problems
9.2 Jacobi Method
*/

#pragma once

#include "identity.h"
#include "matrix.h"
#include "vector.h"

#include <src/com/type/limit.h>

#include <array>
#include <cstddef>
#include <exception>

namespace ns::numerical
{
namespace jacobi_method_implementation
{
template <std::size_t N, typename T>
[[nodiscard]] T threshold(const Matrix<N, N, T>& a)
{
        static_assert(N > 0);
        T sum = 0;
        for (std::size_t i = 0; i < N - 1; ++i)
        {
                for (std::size_t j = i + 1; j < N; ++j)
                {
                        sum += std::abs(a[i, j]);
                }
        }
        return T{0.5} * sum / (N * (N - 1));
}

template <std::size_t N, typename T>
void rotate(
        const std::size_t k,
        const std::size_t l,
        Matrix<N, N, T>* const m,
        std::array<Vector<N, T>, N>* const eigenvectors)
{
        Matrix<N, N, T>& a = *m;

        T t;
        {
                const T diff = a[l, l] - a[k, k];
                const T phi = diff / (2 * a[k, l]);
                const T phi_s = phi * phi + 1;
                if (phi_s <= Limits<T>::max())
                {
                        t = 1 / (std::abs(phi) + std::sqrt(phi_s));
                        t = std::copysign(t, phi);
                }
                else
                {
                        t = 1 / (2 * phi);
                }
        }

        const T c = 1 / std::sqrt(t * t + 1);
        const T s = t * c;
        const T tau = s / (1 + c);

        const T a_kl = a[k, l];
        a[k, l] = 0;
        a[k, k] = a[k, k] - t * a_kl;
        a[l, l] = a[l, l] + t * a_kl;

        for (std::size_t i = 0; i < k; ++i)
        {
                const T a_ik = a[i, k];
                const T a_il = a[i, l];
                a[i, k] = a_ik - s * (a_il + tau * a_ik);
                a[i, l] = a_il + s * (a_ik - tau * a_il);
        }

        for (std::size_t i = k + 1; i < l; ++i)
        {
                const T a_ki = a[k, i];
                const T a_il = a[i, l];
                a[k, i] = a_ki - s * (a_il + tau * a_ki);
                a[i, l] = a_il + s * (a_ki - tau * a_il);
        }

        for (std::size_t i = l + 1; i < N; ++i)
        {
                const T a_ki = a[k, i];
                const T a_li = a[l, i];
                a[k, i] = a_ki - s * (a_li + tau * a_ki);
                a[l, i] = a_li + s * (a_ki - tau * a_li);
        }

        for (std::size_t i = 0; i < N; ++i)
        {
                const T p_ik = (*eigenvectors)[k][i];
                const T p_il = (*eigenvectors)[l][i];
                (*eigenvectors)[k][i] = p_ik - s * (p_il + tau * p_ik);
                (*eigenvectors)[l][i] = p_il + s * (p_ik - tau * p_il);
        }
}
}

class EigenException final : public std::exception
{
        static constexpr const char* MSG = "Jacobi method did not converge";

public:
        [[nodiscard]] const char* what() const noexcept override
        {
                return MSG;
        }
};

template <std::size_t N, typename T>
struct Eigen final
{
        Vector<N, T> values;
        std::array<Vector<N, T>, N> vectors;
};

template <std::size_t N, typename T>
[[nodiscard]] Eigen<N, T> eigen_symmetric_upper_triangular(Matrix<N, N, T> a, const T& tolerance)
{
        static_assert(std::is_floating_point_v<T>);
        namespace impl = jacobi_method_implementation;

        std::array<Vector<N, T>, N> vectors = IDENTITY_ARRAY<N, T>;

        for (int n = 0; n < 20; ++n)
        {
                const T mu = impl::threshold(a);

                for (std::size_t k = 0; k < N - 1; ++k)
                {
                        for (std::size_t l = k + 1; l < N; ++l)
                        {
                                if (std::abs(a[k, l]) >= mu)
                                {
                                        impl::rotate(k, l, &a, &vectors);
                                }
                        }
                }

                if (mu <= tolerance)
                {
                        return {.values = a.diagonal(), .vectors = vectors};
                }
        }

        throw EigenException();
}
}
