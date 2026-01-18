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

#include "adjoint.h"
#include "polynomial.h"
#include "solve.h"

#include <src/com/error.h>
#include <src/numerical/matrix.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace ns::filter::attitude::determination
{
namespace
{
template <typename T>
constexpr T EIGENVALUE_ACCURACY = 1e-5;

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
[[nodiscard]] numerical::Matrix<3, 3, T> make_b_matrix(
        const std::vector<numerical::Vector<3, T>>& observations,
        const std::vector<numerical::Vector<3, T>>& references,
        const std::vector<T>& weights)
{
        const std::size_t size = observations.size();

        numerical::Matrix<3, 3, T> res(numerical::ZERO_MATRIX);
        for (std::size_t n = 0; n < size; ++n)
        {
                const auto& w = weights[n];
                const auto& obs = observations[n];
                const auto& ref = references[n];

                for (std::size_t i = 0; i < 3; ++i)
                {
                        for (std::size_t j = 0; j < 3; ++j)
                        {
                                res[i, j] += w * obs[i] * ref[j];
                        }
                }
        }
        return res;
}

template <typename T>
[[nodiscard]] std::optional<T> largest_eigenvalue_2(
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

        const T v = a0 * a0 + 2 * a0 * a1 * cos + a1 * a1;
        if (v >= 0)
        {
                return std::sqrt(v);
        }

        return std::nullopt;
}

template <typename T>
[[nodiscard]] std::optional<T> largest_eigenvalue(const numerical::Matrix<3, 3, T>& s, const numerical::Vector<3, T>& z)
{
        const numerical::Matrix<3, 3, T> s_adj = adjoint_symmetric(s);

        const T sigma = s.trace() / 2;
        const T kappa = s_adj.trace();
        const T delta = determinant(s, s_adj);

        const numerical::Vector<3, T> s_z = s * z;

        const T a = sigma * sigma - kappa;
        const T b = sigma * sigma + dot(z, z);
        const T c = delta + dot(z, s_z);
        const T d = dot(z, s * s_z);

        const T c0 = -a - b;
        const T c1 = -c;
        const T c2 = a * b + c * sigma - d;

        const CharacteristicPolynomial<T> p({c0, c1, c2});

        return newton_raphson(p, T{1}, EIGENVALUE_ACCURACY<T>);
}

template <typename T>
struct Solve final
{
        numerical::Vector<3, T> z;
        numerical::Matrix<3, 3, T> adj;
        T det;
};

template <typename T>
[[nodiscard]] Solve<T> solve(
        const std::vector<numerical::Vector<3, T>>& obs,
        const std::vector<numerical::Vector<3, T>>& ref,
        const std::vector<T>& w)
{
        ASSERT(obs.size() == ref.size());
        ASSERT(obs.size() == w.size());
        ASSERT(obs.size() >= 2);

        const numerical::Matrix<3, 3, T> b = make_b_matrix(obs, ref, w);
        const numerical::Matrix<3, 3, T> s = b + b.transposed();

        const numerical::Vector<3, T> z{
                b[1, 2] - b[2, 1],
                b[2, 0] - b[0, 2],
                b[0, 1] - b[1, 0],
        };

        const auto l_max = (obs.size() == 2) ? largest_eigenvalue_2(obs, ref, w) : largest_eigenvalue(s, z);
        if (!l_max)
        {
                error("Largest eigenvalue not found");
        }

        const numerical::Matrix<3, 3, T> m = numerical::make_diagonal_matrix<3, T>(*l_max + b.trace()) - s;
        const numerical::Matrix<3, 3, T> m_adj = adjoint_symmetric(m);
        const T m_det = determinant(m, m_adj);

        return {
                .z = z,
                .adj = m_adj,
                .det = m_det,
        };
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

        const Solve<T> s = solve(obs_n, ref_n, w_2);

        return numerical::QuaternionHJ<T, true>(s.adj * s.z, s.det).normalized();
}

#define TEMPLATE(T)                                                                                       \
        template numerical::QuaternionHJ<T, true> quest_attitude(                                         \
                const std::vector<numerical::Vector<3, T>>&, const std::vector<numerical::Vector<3, T>>&, \
                const std::vector<T>&);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
