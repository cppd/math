/*
Copyright (C) 2017-2020 Topological Manifold

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

//   В одномерном массиве хранятся данные по множеству измерений.
// Требуется найти индекс элемента с заданными координатами.
//   Например, имеется 4 измерения с размерами S0, S1, S2, S3
// и условиями S0 > 0 && S1 > 0 && S2 > 0 && S3 > 0,
// и имеется элемент с координатами x0, x1, x2, x3 и условиями
// 0 <= x0 < S0 && 0 <= x1 < S1 && 0 <= x2 < S2 && 0 <= x3 < S3.
// Тогда индекс элемента в массиве будет равен
//      ((x3*S2 + x2)*S1 + x1)*S0 + x0
// или
//      x3*(S2*S1*S0) + x2*(S1*S0) + x1*(S0) + x0.
//   Эти варианты имеют одинаковое количество арифметических действий
// (после предварительного умножения), но второй вариант позволяет
// параллельно умножать, поэтому используется он.
template <size_t N, typename IndexType>
class GlobalIndex
{
        static_assert(N > 0);
        static_assert(is_native_integral<IndexType>);

        template <typename T>
        static constexpr void static_check_input_type()
        {
                // Все размеры должны быть положительными, все координаты неотрицательными,
                // поэтому не требуется, чтобы типы были или оба знаковые, или оба беззнаковые.

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

                // Для x[0] это 1, для x[1] это size[0], для x[2] == size[1] * size[0] и т.д.
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
