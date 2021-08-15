/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "type/detect.h"
#include "type/limit.h"
#include "type/trait.h"

#include <array>
#include <cstddef>
#include <utility>
#include <vector>

namespace ns
{
// index = ((x[3] * s[2] + x[2]) * s[1] + x[1]) * s[0] + x[0]
// index = x[3] * (s[2] * s[1] * s[0]) + x[2] * (s[1] * s[0]) + x[1] * (s[0]) + x[0]
template <std::size_t N, typename IndexType>
class GlobalIndex
{
        static_assert(N > 0);
        static_assert(is_native_integral<IndexType>);

        template <typename T>
        static constexpr void static_check_input_type()
        {
                static_assert((is_array<T> && std::tuple_size_v<T> == N) || is_vector<T>);
                static_assert(is_native_integral<typename T::value_type>);
                static_assert(
                        limits<IndexType>::digits >= limits<typename T::value_type>::digits
                        || (limits<IndexType>::digits >= limits<std::ptrdiff_t>::digits
                            && limits<typename T::value_type>::digits >= limits<std::ptrdiff_t>::digits));
        }

        template <typename T, unsigned... I>
        static constexpr std::array<IndexType, N> compute_strides(
                const T& sizes,
                std::integer_sequence<unsigned, I...>&&)
        {
                static_assert(sizeof...(I) == N);

                static_check_input_type<T>();

                if constexpr (is_vector<T>)
                {
                        ASSERT(sizes.size() == N);
                }

                if (!((sizes[I] > 0) && ...))
                {
                        error("Global index sizes must be positive");
                }

                // 1, size[0], size[1] * size[0]
                IndexType previous = 1;
                std::array<IndexType, N> strides{(I == 0 ? 1 : previous = sizes[I - 1] * previous)...};

                using CheckType = std::conditional_t<is_signed<typename T::value_type>, __int128, unsigned __int128>;
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

        std::array<IndexType, N> m_strides;
        IndexType m_count;

public:
        GlobalIndex() = default;

        template <typename T>
        explicit constexpr GlobalIndex(const T& sizes)
                : m_strides(compute_strides(sizes)), m_count(m_strides[N - 1] * sizes[N - 1])
        {
        }

        constexpr IndexType count() const
        {
                return m_count;
        }

        constexpr IndexType stride(unsigned n) const
        {
                ASSERT(n < N);
                return m_strides[n];
        }

        template <typename T>
        constexpr IndexType compute(const T& p) const
        {
                static_check_input_type<T>();

                if constexpr (is_vector<T>)
                {
                        ASSERT(p.size() == N);
                }

                IndexType global_index = p[0];
                for (unsigned i = 1; i < N; ++i)
                {
                        global_index += m_strides[i] * p[i];
                }

                return global_index;
        }
};
}
