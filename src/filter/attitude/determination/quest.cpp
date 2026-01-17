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

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/matrix.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

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
[[nodiscard]] std::array<T, 3> characteristic_polynomial(
        const numerical::Matrix<3, 3, T>& s,
        const numerical::Vector<3, T>& z)
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

        return {
                -a - b,
                -c,
                a * b + c * sigma - d,
        };
}

template <typename T>
class CharacteristicPolynomial final
{
        // x^4 + c0 * x^2 + c1 * x + c2
        const std::array<T, 3> f_;

        // 4 * x^3 + c0 * x + c1
        const std::array<T, 2> d_;

public:
        explicit CharacteristicPolynomial(const std::array<T, 3>& f)
                : f_(f),
                  d_{2 * f[0], f[1]}
        {
        }

        [[nodiscard]] T f(const T x) const
        {
                return ((x * x + f_[0]) * x + f_[1]) * x + f_[2];
        }

        [[nodiscard]] T d(const T x) const
        {
                return (4 * x * x + d_[0]) * x + d_[1];
        }

        [[nodiscard]] std::string str() const
        {
                const auto s = [](const T v)
                {
                        return (v < 0 ? " - " : " + ") + to_string(std::abs(v));
                };

                std::string res;
                res += "f = x^4";
                if (!(f_[0] == 0))
                {
                        res += s(f_[0]) + " * x^2";
                }
                if (!(f_[1] == 0))
                {
                        res += s(f_[1]) + " * x";
                }
                if (!(f_[2] == 0))
                {
                        res += s(f_[2]);
                }
                res += ", ";
                res += "d = 4 * x^3";
                if (!(d_[0] == 0))
                {
                        res += s(d_[0]) + " * x";
                }
                if (!(d_[1] == 0))
                {
                        res += s(d_[1]);
                }
                return res;
        }
};

template <typename F, typename T>
[[nodiscard]] std::optional<T> newton_raphson(const F& function, const T init, const T accuracy)
{
        constexpr int MAX_ITERATION_COUNT = 15;
        T res = init;
        for (int i = 0; i < MAX_ITERATION_COUNT; ++i)
        {
                const T f = function.f(res);
                const T d = function.d(res);
                const T dx = f / d;
                res -= dx;
                if (std::abs(dx) <= accuracy && std::abs(f) <= accuracy)
                {
                        return res;
                }
        }
        return std::nullopt;
}

template <typename T>
[[nodiscard]] std::optional<T> largest_eigenvalue(
        const std::vector<numerical::Vector<3, T>>& observations,
        const std::vector<numerical::Vector<3, T>>& references,
        const std::vector<T>& weights,
        const numerical::Matrix<3, 3, T>& s,
        const numerical::Vector<3, T>& z)
{
        ASSERT(observations.size() >= 2);
        ASSERT(observations.size() == references.size());
        ASSERT(observations.size() == weights.size());

        if (observations.size() == 2)
        {
                return largest_eigenvalue_2(observations, references, weights);
        }

        const CharacteristicPolynomial p(characteristic_polynomial(s, z));

        return newton_raphson(p, T{1}, EIGENVALUE_ACCURACY<T>);
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

        const auto l_max = largest_eigenvalue(obs_n, ref_n, w_2, s, z);
        if (!l_max)
        {
                error("Largest eigenvalue not found");
        }

        const numerical::Matrix<3, 3, T> m = numerical::make_diagonal_matrix<3, T>(*l_max + sigma) - s;
        const numerical::Matrix<3, 3, T> m_adj = adjoint_symmetric(m);
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
