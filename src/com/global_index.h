/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "alg.h"
#include "error.h"

#include "type/concept.h"
#include "type/limit.h"

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace ns
{
// index = ((x[3] * s[2] + x[2]) * s[1] + x[1]) * s[0] + x[0]
// index = x[3] * (s[2] * s[1] * s[0]) + x[2] * (s[1] * s[0]) + x[1] * (s[0]) + x[0]
template <std::size_t N, typename IndexType>
class GlobalIndex final
{
        static_assert(N > 0);
        static_assert(Integral<IndexType>);

        template <typename T>
        static constexpr void check_input_type()
        {
                static_assert(std::tuple_size_v<T> == N);
                static_assert(Integral<typename T::value_type>);
                static_assert(
                        Limits<IndexType>::digits() >= Limits<typename T::value_type>::digits()
                        || (Limits<IndexType>::digits() >= Limits<std::ptrdiff_t>::digits()
                            && Limits<typename T::value_type>::digits() >= Limits<std::ptrdiff_t>::digits()));
        }

        template <typename T, unsigned... I>
        static constexpr std::array<IndexType, N> compute_strides(
                const T& sizes,
                std::integer_sequence<unsigned, I...>&&)
        {
                static_assert(sizeof...(I) == N);

                check_input_type<T>();

                if (!((sizes[I] > 0) && ...))
                {
                        error("Global index sizes must be positive");
                }

                // 1, size[0], size[1] * size[0]
                IndexType previous = 1;
                std::array<IndexType, N> strides{(I == 0 ? 1 : previous = sizes[I - 1] * previous)...};

                using CheckType = std::conditional_t<Signed<typename T::value_type>, __int128, unsigned __int128>;
                if (static_cast<CheckType>(strides[N - 1]) * static_cast<CheckType>(sizes[N - 1])
                    != multiply_all<CheckType>(sizes))
                {
                        error("Error computing global index strides");
                }

                if (!((I == 0 ? true
                              : ((strides[I] > strides[I - 1] && sizes[I - 1] > 1)
                                 || (strides[I] == strides[I - 1] && sizes[I - 1] == 1)))
                      && ...))
                {
                        error("Error computing global index strides");
                }

                return strides;
        }

        template <typename T, unsigned... I>
        static constexpr std::array<IndexType, N> compute_strides(const T& sizes)
        {
                return compute_strides(sizes, std::make_integer_sequence<unsigned, N>());
        }

        std::array<IndexType, N> strides_;
        IndexType count_;

public:
        GlobalIndex() = default;

        template <typename T>
        explicit constexpr GlobalIndex(const T& sizes)
                : strides_(compute_strides(sizes)),
                  count_(strides_[N - 1] * sizes[N - 1])
        {
        }

        [[nodiscard]] constexpr IndexType count() const
        {
                return count_;
        }

        [[nodiscard]] constexpr IndexType stride(const unsigned n) const
        {
                ASSERT(n < N);
                return strides_[n];
        }

        template <typename T>
        [[nodiscard]] constexpr IndexType compute(const T& p) const
        {
                check_input_type<T>();

                IndexType res = p[0];
                for (std::size_t i = 1; i < N; ++i)
                {
                        res += strides_[i] * p[i];
                }
                return res;
        }
};
}
