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
Åke Björck.
Numerical Methods in Matrix Computations.
Springer, 2015.

2.3 Orthogonal Factorizations
*/

/*
Gene H. Golub, Charles F. Van Loan.
Matrix Computations. Fourth Edition.
The Johns Hopkins University Press, 2013.

5 Orthogonalization and Least Squares
*/

#pragma once

#include <src/com/error.h>
#include <src/com/print.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <span>
#include <type_traits>
#include <vector>

namespace ns::numerical
{
namespace qr_implementation
{
template <typename T>
T norm(const std::vector<T>& x)
{
        ASSERT(!x.empty());

        T res = x[0] * x[0];
        for (std::size_t i = 1; i < x.size(); ++i)
        {
                res += x[i] * x[i];
        }
        return std::sqrt(res);
}

template <typename T>
T dot(const std::vector<T>& a, const std::span<T> b)
{
        ASSERT(a.size() == b.size());
        ASSERT(!a.empty());

        T res = a[0] * b[0];
        for (std::size_t i = 1; i < a.size(); ++i)
        {
                res += a[i] * b[i];
        }
        return res;
}

template <typename T>
void normalize_householder(std::vector<T>& v)
{
        ASSERT(!v.empty());

        const T d = v[0];
        v[0] = 1;
        for (std::size_t i = 1; i < v.size(); ++i)
        {
                v[i] = v[i] / d;
        }
}

template <typename T>
void check_size(const std::vector<std::vector<T>>& a, const std::vector<T>& b)
{
        const std::size_t n = a.size();
        if (n < 1)
        {
                error("Empty system for QR, n = 0");
        }

        const std::size_t m = b.size();
        if (m < n)
        {
                error("Underdetermined system for QR, m (" + to_string(m) + ") is less than n (" + to_string(n) + ")");
        }

        for (const std::vector<T>& v : a)
        {
                if (v.size() != b.size())
                {
                        error("Vector sizes are not equal, " + to_string(v.size()) + " != m (" + to_string(m) + ")");
                }
        }
}

template <typename T>
struct HouseholderReflection final
{
        T beta;
        T sigma;
        std::vector<T> v;
};

template <typename T>
void householder_reflection(HouseholderReflection<T>& hr)
{
        if (hr.v.empty())
        {
                error("Empty vector for Householder reflection");
        }

        T& beta = hr.beta;
        T& sigma = hr.sigma;
        std::vector<T>& v = hr.v;

        const T v0 = v[0];

        sigma = norm(v);
        v[0] = sigma + abs(v0);
        beta = v[0] / sigma;

        if (v0 < 0)
        {
                v[0] = -v[0];
        }
        else
        {
                sigma = -sigma;
        }

        normalize_householder(v);
}

template <typename T>
void reflect(const HouseholderReflection<T>& hr, const std::span<T> x)
{
        ASSERT(x.size() == hr.v.size());
        ASSERT(!x.empty());

        // P * A = (I - beta * v * v^T) * A
        // P * A = A - (beta * v) * (v^T * A)

        const T k = hr.beta * dot(hr.v, x);
        for (std::size_t i = 0; i < x.size(); ++i)
        {
                x[i] -= k * hr.v[i];
        }
}

template <typename T>
void solve_x(std::vector<std::vector<T>>& a, std::vector<T>& b)
{
        using Signed = std::make_signed_t<std::size_t>;

        const Signed n = a.size();
        ASSERT(n >= 1);

        // Back substitution.
        // a (upper triangular matrix) * x = b

        b[n - 1] /= a[n - 1][n - 1];
        for (Signed k = n - 2; k >= 0; --k)
        {
                for (Signed j = k + 1; j < n; ++j)
                {
                        b[k] -= a[j][k] * b[j];
                }
                b[k] /= a[k][k];
        }
}
}

template <typename T>
[[nodiscard]] std::vector<T> qr_solve(std::vector<std::vector<T>>& a, std::vector<T>& b)
{
        namespace impl = qr_implementation;

        impl::check_size(a, b);

        const std::size_t n = a.size();
        const std::size_t m = b.size();

        impl::HouseholderReflection<T> hr;

        ASSERT(m > 0);
        const std::size_t max_col = std::min(m - 1, n);

        for (std::size_t col = 0; col < max_col; ++col)
        {
                hr.v.resize(m - col);
                std::copy(a[col].begin() + col, a[col].end(), hr.v.begin());
                impl::householder_reflection(hr);

                a[col][col] = hr.sigma;
                for (std::size_t j = col + 1; j < n; ++j)
                {
                        impl::reflect(hr, std::span(a[j].begin() + col, a[j].end()));
                }
                impl::reflect(hr, std::span(b.begin() + col, b.end()));
        }

        impl::solve_x(a, b);

        return {b.begin(), b.begin() + n};
}
}
