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
#include <vector>

namespace ns::numerical
{
template <typename T>
struct HouseholderReflection final
{
        T beta;
        T sigma;
        std::vector<T> v;
};

template <typename T>
HouseholderReflection<T> householder_reflection(const std::vector<T>& x)
{
        if (x.empty())
        {
                error("Empty vector for Householder reflection");
        }

        const T norm = [&]
        {
                T sum = 0;
                for (const T& v : x)
                {
                        sum += v * v;
                }
                return std::sqrt(sum);
        }();

        std::vector<T> u = x;
        T sigma = norm;
        u[0] = sigma + abs(x[0]);
        const T beta = u[0] / sigma;

        if (x[0] < 0)
        {
                u[0] = -u[0];
        }
        else
        {
                sigma = -sigma;
        }

        const T d = u[0];
        u[0] = 1;
        for (std::size_t i = 1; i < u.size(); ++i)
        {
                u[i] = u[i] / d;
        }

        return {
                .beta = beta,
                .sigma = sigma,
                .v = std::move(u),
        };
}
}
