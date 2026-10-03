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

        T sum = x[0] * x[0];
        for (std::size_t i = 1; i < x.size(); ++i)
        {
                sum += x[i] * x[i];
        }
        return std::sqrt(sum);
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
void solve_x(std::vector<std::vector<T>>& a, std::vector<T>& b)
{
        using Signed = std::make_signed_t<std::size_t>;

        const Signed n = a.size();
        ASSERT(n >= 1);

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
struct HouseholderReflection final
{
        T beta;
        T sigma;
        std::vector<T> v;
};

template <typename T>
void householder_reflection(HouseholderReflection<T>& hr)
{
        namespace impl = qr_implementation;

        T& beta = hr.beta;
        T& sigma = hr.sigma;
        std::vector<T>& x = hr.v;

        if (x.empty())
        {
                error("Empty vector for Householder reflection");
        }

        const T v0 = x[0];

        sigma = impl::norm(x);
        x[0] = sigma + abs(v0);
        beta = x[0] / sigma;

        if (v0 < 0)
        {
                x[0] = -x[0];
        }
        else
        {
                sigma = -sigma;
        }

        const T d = x[0];
        x[0] = 1;
        for (std::size_t i = 1; i < x.size(); ++i)
        {
                x[i] = x[i] / d;
        }
}

template <typename T>
void reflect(const HouseholderReflection<T>& hr, const std::span<T> x)
{
        namespace impl = qr_implementation;

        ASSERT(x.size() == hr.v.size());
        ASSERT(!x.empty());

        // P * A = (I - beta * v * v^T) * A
        // P * A = A - (beta * v) * (v^T * A)

        const T k = hr.beta * impl::dot(hr.v, x);
        for (std::size_t i = 0; i < x.size(); ++i)
        {
                x[i] -= k * hr.v[i];
        }
}

template <typename T>
void solve_qr(std::vector<std::vector<T>>& a, std::vector<T>& b)
{
        namespace impl = qr_implementation;

        impl::check_size(a, b);

        const std::size_t n = a.size();
        const std::size_t m = b.size();

        HouseholderReflection<T> hr;

        ASSERT(m > 0);
        const std::size_t max_col = std::min(m - 1, n);

        for (std::size_t col = 0; col < max_col; ++col)
        {
                hr.v.resize(m - col);
                std::copy(a[col].begin() + col, a[col].end(), hr.v.begin());
                householder_reflection(hr);

                a[col][col] = hr.sigma;
                for (std::size_t j = col + 1; j < n; ++j)
                {
                        reflect(hr, std::span(a[j].begin() + col, a[j].end()));
                }
                reflect(hr, std::span(b.begin() + col, b.end()));
        }

        impl::solve_x(a, b);

        b.resize(n);
}
}
