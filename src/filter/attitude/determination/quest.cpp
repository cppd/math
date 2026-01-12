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

#include "quest.h"

#include <src/com/error.h>
#include <src/numerical/matrix.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <vector>

namespace ns::filter::attitude::determination
{
namespace
{
template <typename T>
[[nodiscard]] std::vector<numerical::Vector<3, T>> normalize(const std::vector<numerical::Vector<3, T>>& values)
{
        std::vector<numerical::Vector<3, T>> res;
        res.reserve(values.size());
        for (const auto& v : values)
        {
                res.push_back(v.normalized());
        }
        return res;
}

template <typename T>
[[nodiscard]] std::vector<T> normalize_weights(std::vector<T> weights)
{
        T sum = 0;
        for (T& v : weights)
        {
                v *= v;
                sum += v;
        }
        for (T& v : weights)
        {
                v /= sum;
        }
        return weights;
}

template <typename T>
[[nodiscard]] numerical::Matrix<3, 3, T> make_b(
        const std::vector<numerical::Vector<3, T>>& observations,
        const std::vector<numerical::Vector<3, T>>& references,
        const std::vector<T>& weights)
{
        const std::size_t size = observations.size();
        numerical::Matrix<3, 3, T> b(numerical::ZERO_MATRIX);
        for (std::size_t n = 0; n < size; ++n)
        {
                for (std::size_t i = 0; i < 3; ++i)
                {
                        for (std::size_t j = 0; j < 3; ++j)
                        {
                                b[i, j] += weights[n] * observations[n][i] * references[n][j];
                        }
                }
        }
        return b;
}

template <typename T>
[[nodiscard]] T largest_eigenvalue_2(
        const std::vector<numerical::Vector<3, T>>& observations,
        const std::vector<numerical::Vector<3, T>>& references,
        const std::vector<T>& weights)
{
        ASSERT(observations.size() == 2);
        ASSERT(references.size() == 2);
        ASSERT(weights.size() == 2);

        const T rd = dot(references[0], references[1]);
        const T od = dot(observations[0], observations[1]);

        const T rc = cross(references[0], references[1]).norm();
        const T oc = cross(observations[0], observations[1]).norm();

        const T cos = rd * od + rc * oc;
        const T a0 = weights[0];
        const T a1 = weights[1];

        return std::sqrt(a0 * a0 + 2 * a0 * a1 * cos + a1 * a1);
}

template <typename T>
[[nodiscard]] numerical::Matrix<3, 3, T> adjoint(const numerical::Matrix<3, 3, T>& m)
{
        numerical::Matrix<3, 3, T> res;

        res[0, 0] = +(m[1, 1] * m[2, 2] - m[1, 2] * m[2, 1]);
        res[0, 1] = -(m[0, 1] * m[2, 2] - m[0, 2] * m[2, 1]);
        res[0, 2] = +(m[0, 1] * m[1, 2] - m[0, 2] * m[1, 1]);

        res[1, 0] = -(m[1, 0] * m[2, 2] - m[1, 2] * m[2, 0]);
        res[1, 1] = +(m[0, 0] * m[2, 2] - m[0, 2] * m[2, 0]);
        res[1, 2] = -(m[0, 0] * m[1, 2] - m[0, 2] * m[1, 0]);

        res[2, 0] = +(m[1, 0] * m[2, 1] - m[1, 1] * m[2, 0]);
        res[2, 1] = -(m[0, 0] * m[2, 1] - m[0, 1] * m[2, 0]);
        res[2, 2] = +(m[0, 0] * m[1, 1] - m[0, 1] * m[1, 0]);

        return res;
}

template <typename T>
[[nodiscard]] T determinant(const numerical::Matrix<3, 3, T>& m, const numerical::Matrix<3, 3, T>& adj)
{
        return m[0, 0] * adj[0, 0] + m[0, 1] * adj[1, 0] + m[0, 2] * adj[2, 0];
}
}

template <typename T>
[[nodiscard]] numerical::QuaternionHJ<T, true> quest_attitude(
        const std::vector<numerical::Vector<3, T>>& observations,
        const std::vector<numerical::Vector<3, T>>& references,
        const std::vector<T>& weights)
{
        if (observations.size() != references.size() || observations.size() != weights.size())
        {
                error("Not equal sizes");
        }

        if (observations.size() < 2)
        {
                error("At least 2 observations are required");
        }

        const std::vector<numerical::Vector<3, T>> obs_n = normalize(observations);
        const std::vector<numerical::Vector<3, T>> ref_n = normalize(references);
        const std::vector<T> w_2 = normalize_weights(weights);

        const numerical::Matrix<3, 3, T> b = make_b(obs_n, ref_n, w_2);

        const T sigma = b.trace();

        const numerical::Matrix<3, 3, T> s = b + b.transposed();

        const numerical::Vector<3, T> z{
                b[1, 2] - b[2, 1],
                b[2, 0] - b[0, 2],
                b[0, 1] - b[1, 0],
        };

        if (obs_n.size() > 2)
        {
                error("More than 2 observations not implemented");
        }

        const T l_max = largest_eigenvalue_2(obs_n, ref_n, w_2);

        const numerical::Matrix<3, 3, T> m = numerical::make_diagonal_matrix<3, T>(l_max + sigma) - s;
        const numerical::Matrix<3, 3, T> m_adj = adjoint(m);
        const T m_det = determinant(m, m_adj);

        return numerical::QuaternionHJ<T, true>(m_adj * z, m_det).normalized();
}

#define TEMPLATE(T)                                                                                       \
        template numerical::QuaternionHJ<T, true> quest_attitude(                                         \
                const std::vector<numerical::Vector<3, T>>&, const std::vector<numerical::Vector<3, T>>&, \
                const std::vector<T>&);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
