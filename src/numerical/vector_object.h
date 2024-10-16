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

/*
Steven J. Leon.
Linear Algebra with Applications. Ninth Edition.
Pearson Education, 2015.

5.4 Inner Product Spaces
Norms
*/

#pragma once

#include <src/com/exponent.h>
#include <src/com/hash.h>
#include <src/com/interpolation.h>
#include <src/com/type/limit.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace ns::numerical
{
template <std::size_t N, typename T>
class Vector final
{
        static_assert(N > 0);

        std::array<T, N> data_;

        template <std::size_t... I>
        constexpr Vector(std::integer_sequence<std::size_t, I...>&&, const T& v)
                : data_{(static_cast<void>(I), v)...}
        {
                static_assert(sizeof...(I) == N);
        }

public:
        constexpr Vector()
        {
        }

        template <typename Arg1, typename Arg2, typename... Args>
        constexpr Vector(Arg1&& arg1, Arg2&& arg2, Args&&... args)
                : data_{static_cast<T>(std::forward<Arg1>(arg1)), static_cast<T>(std::forward<Arg2>(arg2)),
                        static_cast<T>(std::forward<Args>(args))...}
        {
                static_assert(sizeof...(args) + 2 == N);
        }

        explicit constexpr Vector(const std::type_identity_t<T>& v)
                : Vector(std::make_integer_sequence<std::size_t, N>(), v)
        {
        }

        [[nodiscard]] constexpr const T& operator[](const std::size_t i) const
        {
                return data_[i];
        }

        [[nodiscard]] constexpr T& operator[](const std::size_t i)
        {
                return data_[i];
        }

        [[nodiscard]] const T* data() const
        {
                static_assert(sizeof(Vector) == N * sizeof(T));
                return data_.data();
        }

        [[nodiscard]] std::size_t hash() const
        {
                return compute_hash(data_);
        }

        [[nodiscard]] constexpr Vector<N, T> operator-() const
        {
                Vector<N, T> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] = -data_[i];
                }
                return res;
        }

        constexpr Vector<N, T>& operator+=(const Vector<N, T>& a) &
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        data_[i] += a[i];
                }
                return *this;
        }

        constexpr Vector<N, T>& operator-=(const Vector<N, T>& a) &
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        data_[i] -= a[i];
                }
                return *this;
        }

        constexpr Vector<N, T>& operator*=(const Vector<N, T>& a) &
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        data_[i] *= a[i];
                }
                return *this;
        }

        constexpr Vector<N, T>& operator*=(const T& v) &
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        data_[i] *= v;
                }
                return *this;
        }

        constexpr Vector<N, T>& operator/=(const T& v) &
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        data_[i] /= v;
                }
                return *this;
        }

        constexpr void multiply_add(const Vector<N, T>& a, const T& b)
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        data_[i] += a[i] * b;
                }
        }

        constexpr void multiply_add(const Vector<N, T>& a, const Vector<N, T>& b)
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        data_[i] += a[i] * b[i];
                }
        }

        constexpr void multiply_add(const T& b, const Vector<N, T>& a)
        {
                multiply_add(a, b);
        }

        [[nodiscard]] T norm_1() const
        {
                T res = std::abs(data_[0]);
                for (std::size_t i = 1; i < N; ++i)
                {
                        res += std::abs(data_[i]);
                }
                return res;
        }

        [[nodiscard]] T norm_infinity() const
        {
                T res = std::abs(data_[0]);
                for (std::size_t i = 1; i < N; ++i)
                {
                        res = std::max(std::abs(data_[i]), res);
                }
                return res;
        }

        [[nodiscard]] T norm_squared() const
        {
                T s = data_[0] * data_[0];
                for (std::size_t i = 1; i < N; ++i)
                {
                        s += data_[i] * data_[i];
                }
                return s;
        }

        [[nodiscard]] T norm() const
        {
                return std::sqrt(norm_squared());
        }

        [[nodiscard]] T norm_stable() const
        {
                const T max = norm_infinity();

                T k = data_[0] / max;
                T s = k * k;
                for (std::size_t i = 1; i < N; ++i)
                {
                        k = data_[i] / max;
                        s += k * k;
                }
                return max * std::sqrt(s);
        }

        void normalize()
        {
                const T n = norm();
                for (std::size_t i = 0; i < N; ++i)
                {
                        data_[i] /= n;
                }
        }

        [[nodiscard]] Vector<N, T> normalized() const
        {
                const T n = norm();
                Vector<N, T> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] = data_[i] / n;
                }
                return res;
        }

        [[nodiscard]] bool is_zero() const
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (data_[i] == 0)
                        {
                                continue;
                        }
                        return false;
                }
                return true;
        }

        [[nodiscard]] bool is_unit() const
        {
                static constexpr T D = 100 * Limits<T>::epsilon();
                static constexpr T MIN = square(1 - D);
                static constexpr T MAX = square(1 + D);
                const T s = norm_squared();
                return s > MIN && s < MAX;
        }

        [[nodiscard]] constexpr Vector<N, T> clamp(const T& low, const T& high) const
        {
                Vector<N, T> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] = std::clamp(data_[i], low, high);
                }
                return res;
        }

        [[nodiscard]] constexpr Vector<N, T> max_n(const T& v) const
        {
                Vector<N, T> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        // v if data_[i] is NaN
                        res[i] = std::max(v, data_[i]);
                }
                return res;
        }

        [[nodiscard]] constexpr Vector<N, T> reciprocal() const
        {
                Vector<N, T> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        // 1 / -0 == -infinity
                        res[i] = (data_[i] == 0) ? std::numeric_limits<T>::infinity() : (1 / data_[i]);
                }
                return res;
        }

        [[nodiscard]] constexpr Vector<N, bool> negative_bool() const
        {
                Vector<N, bool> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] = (data_[i] < 0);
                }
                return res;
        }

        template <std::size_t POSITION, std::size_t COUNT>
        [[nodiscard]] constexpr Vector<COUNT, T> segment() const
        {
                static_assert(POSITION < N);
                static_assert(COUNT > 0 && COUNT <= N);
                static_assert(POSITION + COUNT <= N);

                Vector<COUNT, T> res;
                for (std::size_t i = 0, j = POSITION; i < COUNT; ++i, ++j)
                {
                        res[i] = data_[j];
                }
                return res;
        }

        template <std::size_t M>
        [[nodiscard]] constexpr Vector<M, T> head() const
        {
                static_assert(M <= N);

                return segment<0, M>();
        }

        template <std::size_t M>
        [[nodiscard]] constexpr Vector<M, T> tail() const
        {
                static_assert(M <= N);

                return segment<N - M, M>();
        }

        [[nodiscard]] friend bool is_finite(const Vector<N, T>& v)
                requires (std::is_floating_point_v<T>)
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (std::isfinite(v[i]))
                        {
                                continue;
                        }
                        return false;
                }
                return true;
        }

        [[nodiscard]] friend constexpr Vector<N, T> interpolation(
                const Vector<N, T>& a,
                const Vector<N, T>& b,
                const T& t)
        {
                Vector<N, T> res;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[i] = interpolation(a[i], b[i], t);
                }
                return res;
        }

        [[nodiscard]] friend std::string to_string(const Vector<N, T>& v)
        {
                std::ostringstream oss;
                oss.precision(Limits<T>::max_digits10());
                oss << '(';
                oss << v[0];
                for (std::size_t i = 1; i < N; ++i)
                {
                        oss << ", " << v[i];
                }
                oss << ')';
                return oss.str();
        }
};
}
