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

#include "gaussian.h"

#include <src/com/exponent.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <optional>

namespace ns::filter::core
{
template <std::size_t N, typename T>
struct UpdateInfo final
{
        numerical::Vector<N, T> residual;
        bool gate = false;
        std::optional<T> normalized_innovation_squared;
        std::optional<T> likelihood;
};

template <std::size_t N, typename T>
[[nodiscard]] UpdateInfo<N, T> make_update_info(const numerical::Vector<N, T>& residual)
{
        UpdateInfo<N, T> res;
        res.residual = residual;
        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] UpdateInfo<N, T> make_update_info(
        const numerical::Vector<N, T>& residual,
        const numerical::Matrix<N, N, T>& s,
        const numerical::Matrix<N, N, T>& s_inversed,
        const std::optional<T> gate,
        const bool likelihood,
        const bool normalized_innovation)
{
        UpdateInfo<N, T> res;

        res.residual = residual;

        if (!(gate || likelihood || normalized_innovation))
        {
                return res;
        }

        const T mahalanobis_distance_squared = compute_mahalanobis_distance_squared(residual, s_inversed);

        if (gate || normalized_innovation)
        {
                res.normalized_innovation_squared = mahalanobis_distance_squared;
        }

        if (likelihood)
        {
                res.likelihood = compute_likelihood(mahalanobis_distance_squared, s);
        }

        if (gate && !(mahalanobis_distance_squared <= square(*gate)))
        {
                res.gate = true;
        }

        return res;
}
}
