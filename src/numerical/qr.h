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

#include <cmath>
#include <cstddef>
#include <span>
#include <vector>

namespace ns::numerical
{
namespace qr_implementation
{
template <typename T>
T norm(const std::vector<T>& x)
{
        T sum = 0;
        for (const T& v : x)
        {
                sum += v * v;
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
void reflect_first(const HouseholderReflection<T>& hr, const std::span<T> x)
{
        namespace impl = qr_implementation;

        ASSERT(x.size() == hr.v.size());
        ASSERT(!x.empty());

        // P * A = (I - beta * v * v^T) * A
        // P * A = A - (beta * v) * (v^T * A)
        const T k = hr.beta * impl::dot(hr.v, x);
        x[0] -= k * hr.v[0];
}
}
