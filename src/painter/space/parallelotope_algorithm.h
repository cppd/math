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

#include <src/com/error.h>
#include <src/com/type/limit.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <array>
#include <type_traits>

namespace painter
{
namespace parallelotope_algorithm_implementation
{
template <int INDEX, template <size_t N, typename T> typename Parallelotope, size_t N, typename T, typename F>
void diagonals_impl(const Parallelotope<N, T>& p, const Vector<N, T>& edge_sum, const F& f)
{
        if constexpr (INDEX >= 0)
        {
                diagonals_impl<INDEX - 1>(p, edge_sum + p.e(INDEX), f);
                diagonals_impl<INDEX - 1>(p, edge_sum - p.e(INDEX), f);
        }
        else
        {
                f(edge_sum);
        }
}

template <template <size_t N, typename T> typename Parallelotope, size_t N, typename T, typename F>
void diagonals(const Parallelotope<N, T>& p, const F& f)
{
        constexpr int LAST_INDEX = N - 1;

        // Перебрать все диагонали одной из граней параллелотопа с учётом их направления.
        // Количество таких диагоналей равно 2 ^ (N - 1). Добавляя к каждой такой
        // диагонали оставшееся измерение, получаются все диагонали целого параллелотопа.
        // Одно из измерений не меняется, остальные к нему прибавляются и вычитаются.
        diagonals_impl<LAST_INDEX - 1>(p, p.e(LAST_INDEX), f);
}

//

template <int INDEX, template <size_t N, typename T> typename Parallelotope, size_t N, typename T, typename F>
void vertex_ridges_impl(
        const Parallelotope<N, T>& p,
        const Vector<N, T>& org,
        std::array<bool, N>& dimensions,
        const F& f)
{
        if constexpr (INDEX >= 0)
        {
                dimensions[INDEX] = false;
                vertex_ridges_impl<INDEX - 1>(p, org, dimensions, f);

                dimensions[INDEX] = true;
                vertex_ridges_impl<INDEX - 1>(p, org + p.e(INDEX), dimensions, f);
        }
        else
        {
                for (unsigned i = 0; i < dimensions.size(); ++i)
                {
                        if (!dimensions[i])
                        {
                                f(org, p.e(i));
                        }
                }
        }
}

template <template <size_t N, typename T> typename Parallelotope, size_t N, typename T, typename F>
void vertex_ridges(const Parallelotope<N, T>& p, const F& f)
{
        constexpr int LAST_INDEX = N - 1;

        std::array<bool, N> dimensions;

        // Смещаться по каждому измерению для перехода к другой вершине.
        // Добавлять к массиву рёбер пары, состоящие из вершины и векторов
        // измерений, по которым не смещались для перехода к этой вершине.
        vertex_ridges_impl<LAST_INDEX>(p, p.org(), dimensions, f);
}
}

template <typename Parallelotope>
class ParallelotopeTraits final
{
        static constexpr size_t N = Parallelotope::DIMENSION;
        using T = typename Parallelotope::DataType;

        static_assert(N <= 27);

        static constexpr int DIAGONAL_COUNT = 1 << (N - 1);

        // Количество вершин 2 ^ N умножить на количество измерений N у каждой вершины
        // и для уникальности разделить на 2 = ((2 ^ N) * N) / 2 = (2 ^ (N - 1)) * N
        static constexpr int VERTEX_RIDGE_COUNT = (1 << (N - 1)) * N;

public:
        // Каждый элемент массива - это вектор произвольного по знаку направления
        using Diagonals = std::array<Vector<N, T>, DIAGONAL_COUNT>;

        // Каждый элемент массива - это вершина откуда и вектор куда
        using VertexRidges = std::array<std::array<Vector<N, T>, 2>, VERTEX_RIDGE_COUNT>;
};

template <template <size_t N, typename T> typename Parallelotope, size_t N, typename T>
T parallelotope_max_diagonal(const Parallelotope<N, T>& p)
{
        namespace impl = parallelotope_algorithm_implementation;

        static_assert(std::is_floating_point_v<T>);

        T max_length = limits<T>::lowest();

        auto f = [&max_length](const Vector<N, T>& d) { max_length = std::max(max_length, d.norm()); };

        impl::diagonals(p, f);

        return max_length;
}

template <template <size_t N, typename T> typename Parallelotope, size_t N, typename T>
typename ParallelotopeTraits<Parallelotope<N, T>>::Diagonals parallelotope_diagonals(const Parallelotope<N, T>& p)
{
        namespace impl = parallelotope_algorithm_implementation;

        typename ParallelotopeTraits<Parallelotope<N, T>>::Diagonals result;

        unsigned diagonal_count = 0;

        auto f = [&diagonal_count, &result](const Vector<N, T>& d) {
                ASSERT(diagonal_count < result.size());
                result[diagonal_count++] = d;
        };

        impl::diagonals(p, f);

        ASSERT(diagonal_count == result.size());

        return result;
}

template <template <size_t N, typename T> typename Parallelotope, size_t N, typename T>
typename ParallelotopeTraits<Parallelotope<N, T>>::VertexRidges parallelotope_vertex_ridges(
        const Parallelotope<N, T>& p)
{
        namespace impl = parallelotope_algorithm_implementation;

        typename ParallelotopeTraits<Parallelotope<N, T>>::VertexRidges result;

        unsigned ridge_count = 0;

        auto f = [&ridge_count, &result](const Vector<N, T>& org, const Vector<N, T>& ridge) {
                ASSERT(ridge_count < result.size());
                result[ridge_count++] = {org, ridge};
        };

        impl::vertex_ridges(p, f);

        ASSERT(ridge_count == result.size());

        return result;
}
}
