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

#include "matrix.h"
#include "polynomial.h"
#include "rotation.h"
#include "solve.h"

#include <src/com/error.h>
#include <src/numerical/matrix.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace ns::filter::attitude::determination
{
namespace
{
template <typename T>
constexpr T EIGENVALUE_ACCURACY = 1e-6;

template <typename T>
using Quaternion = numerical::QuaternionHJ<T, true>;

template <typename T>
[[nodiscard]] std::vector<numerical::Vector<3, T>> normalize_vectors(const std::vector<numerical::Vector<3, T>>& values)
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
[[nodiscard]] std::vector<T> square_and_normalize_weights(const std::vector<T>& weights)
{
        std::vector<T> res;
        res.reserve(weights.size());
        T sum = 0;
        for (const T w : weights)
        {
                const T w2 = w * w;
                sum += w2;
                res.push_back(w2);
        }
        for (T& w : res)
        {
                w /= sum;
        }
        return res;
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
class Solve final
{
        numerical::Vector<3, T> z_;
        numerical::Matrix<3, 3, T> adj_;
        T det_;

public:
        Solve(const std::vector<numerical::Vector<3, T>>& obs,
              const std::vector<numerical::Vector<3, T>>& ref,
              const std::vector<T>& w)
        {
                ASSERT(obs.size() == ref.size());
                ASSERT(obs.size() == w.size());
                ASSERT(obs.size() >= 2);

                const numerical::Matrix<3, 3, T> b = make_b_matrix(obs, ref, w);
                const numerical::Matrix<3, 3, T> s = sum_with_transpose(b);

                const numerical::Vector<3, T> z = {
                        b[1, 2] - b[2, 1],
                        b[2, 0] - b[0, 2],
                        b[0, 1] - b[1, 0],
                };

                const auto l_max = (obs.size() == 2) ? largest_eigenvalue_2(obs, ref, w) : largest_eigenvalue(s, z);
                if (!l_max)
                {
                        error("Largest eigenvalue not found");
                }

                const numerical::Matrix<3, 3, T> m = negate_and_add_diagonal(s, *l_max + b.trace());

                z_ = z;
                adj_ = adjoint_symmetric(m);
                det_ = determinant(m, adj_);
        }

        [[nodiscard]] Quaternion<T> compute() const
        {
                return Quaternion<T>(adj_ * z_, det_).normalized();
        }

        [[nodiscard]] bool operator<(const Solve& s) const
        {
                return std::abs(det_) < std::abs(s.det_);
        }
};
}

template <typename T>
[[nodiscard]] Quaternion<T> quest_attitude(
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

        const std::vector<numerical::Vector<3, T>> obs_n = normalize_vectors(observations);
        const std::vector<numerical::Vector<3, T>> ref_n = normalize_vectors(references);
        const std::vector<T> w_2 = square_and_normalize_weights(weights);

        const std::array s{
                Solve<T>{obs_n, rotate_axis<0>(ref_n), w_2},
                Solve<T>{obs_n, rotate_axis<1>(ref_n), w_2},
                Solve<T>{obs_n, rotate_axis<2>(ref_n), w_2},
                Solve<T>{obs_n,                 ref_n, w_2},
        };

        const std::size_t index = std::max_element(s.begin(), s.end()) - s.cbegin();
        ASSERT(index >= 0 && index <= 3);

        const Quaternion<T> attitude = s[index].compute();

        return (index < 3) ? rotate_axis(attitude, index) : attitude;
}

#define TEMPLATE(T)                                                                                       \
        template Quaternion<T> quest_attitude(                                                            \
                const std::vector<numerical::Vector<3, T>>&, const std::vector<numerical::Vector<3, T>>&, \
                const std::vector<T>&);

TEMPLATE(float)
TEMPLATE(double)
TEMPLATE(long double)
}
