/*
Copyright (C) 2017 Topological Manifold

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

#include "com/error.h"
#include "com/ray.h"
#include "com/vec.h"

#include <array>
#include <limits>
#include <type_traits>

template <typename Parallelotope>
class ParallelotopeAlgorithm final
{
        static constexpr size_t N = Parallelotope::DIMENSION;
        using T = typename Parallelotope::DataType;

        static_assert(N <= 27);

        static constexpr int VERTEX_COUNT = 1 << N;
        static constexpr int DIAGONAL_COUNT = 1 << (N - 1);

        // Количество вершин 2 ^ N умножить на количество измерений N у каждой вершины
        // и для уникальности разделить на 2 = ((2 ^ N) * N) / 2 = (2 ^ (N - 1)) * N
        static constexpr int VERTEX_RIDGE_COUNT = (1 << (N - 1)) * N;

        //

        template <int n, typename F>
        static void diagonals_impl(const Parallelotope& p, const Vector<N, T>& edge_sum, const F& f)
        {
                if constexpr (n >= 0)
                {
                        diagonals_impl<n - 1>(p, edge_sum + p.e(n), f);
                        diagonals_impl<n - 1>(p, edge_sum - p.e(n), f);
                }
                else
                {
                        f(edge_sum);
                }
        }
        template <typename F>
        static void diagonals(const Parallelotope& p, const F& f)
        {
                constexpr int last_index = N - 1;

                // Перебрать все диагонали одной из граней параллелотопа с учётом их направления.
                // Количество таких диагоналей равно 2 ^ (N - 1). Добавляя к каждой такой
                // диагонали оставшееся измерение, получаются все диагонали целого параллелотопа.

                // Одно из измерений не меняется, остальные к нему прибавляются и вычитаются
                diagonals_impl<last_index - 1>(p, p.e(last_index), f);
        }

        //

        template <int n, typename F>
        static void vertices_impl(const Parallelotope& p, const Vector<N, T>& org, const F& f)
        {
                if constexpr (n >= 0)
                {
                        vertices_impl<n - 1>(p, org, f);
                        vertices_impl<n - 1>(p, org + p.e(n), f);
                }
                else
                {
                        f(org);
                }
        }
        template <typename F>
        static void vertices(const Parallelotope& p, const F& f)
        {
                constexpr int last_index = N - 1;

                // Смещаться по каждому измерению для перехода к другой вершине.

                vertices_impl<last_index>(p, p.org(), f);
        }

        //

        template <int n, typename F>
        static void vertex_ridges_impl(const Parallelotope& p, const Vector<N, T>& org, std::array<bool, N>& dimensions,
                                       const F& f)
        {
                if constexpr (n >= 0)
                {
                        dimensions[n] = false;
                        vertex_ridges_impl<n - 1>(p, org, dimensions, f);

                        dimensions[n] = true;
                        vertex_ridges_impl<n - 1>(p, org + p.e(n), dimensions, f);
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
        template <typename F>
        static void vertex_ridges(const Parallelotope& p, const F& f)
        {
                constexpr int last_index = N - 1;

                std::array<bool, N> dimensions;

                // Смещаться по каждому измерению для перехода к другой вершине.
                // Добавлять к массиву рёбер пары, состоящие из вершины и векторов
                // измерений, по которым не смещались для перехода к этой вершине.

                vertex_ridges_impl<last_index>(p, p.org(), dimensions, f);
        }

public:
        // Каждый элемент массива - это вектор произвольного по знаку направления
        using Diagonals = std::array<Vector<N, T>, DIAGONAL_COUNT>;

        // Каждый элемент массива - это точка в пространстве
        using Vertices = std::array<Vector<N, T>, VERTEX_COUNT>;

        // Каждый элемент массива - это вершина откуда и вектор куда
        using VertexRidges = std::array<std::array<Vector<N, T>, 2>, VERTEX_RIDGE_COUNT>;

        static T max_diagonal(const Parallelotope& p)
        {
                static_assert(std::is_floating_point_v<T>);

                T max_length = std::numeric_limits<T>::lowest();

                diagonals(p, [&max_length](const Vector<N, T>& d) { max_length = std::max(max_length, length(d)); });

                return max_length;
        }

        static Diagonals diagonals(const Parallelotope& p)
        {
                Diagonals result;

                unsigned diagonal_count = 0;

                diagonals(p, [&diagonal_count, &result](const Vector<N, T>& d) {
                        ASSERT(diagonal_count < result.size());
                        result[diagonal_count++] = d;
                });

                ASSERT(diagonal_count == result.size());

                return result;
        }

        static Vertices vertices(const Parallelotope& p)
        {
                Vertices result;

                unsigned vertex_count = 0;

                vertices(p, [&vertex_count, &result](const Vector<N, T>& org) {
                        ASSERT(vertex_count < result.size());
                        result[vertex_count++] = org;
                });

                ASSERT(vertex_count == result.size());

                return result;
        }

        static VertexRidges vertex_ridges(const Parallelotope& p)
        {
                VertexRidges result;

                unsigned ridge_count = 0;

                vertex_ridges(p, [&ridge_count, &result](const Vector<N, T>& org, const Vector<N, T>& ridge) {
                        ASSERT(ridge_count < result.size());
                        result[ridge_count++] = {{org, ridge}};
                });

                ASSERT(ridge_count == result.size());

                return result;
        }
};

template <typename Parallelotope>
typename Parallelotope::DataType parallelotope_max_diagonal(const Parallelotope& p)
{
        return ParallelotopeAlgorithm<Parallelotope>::max_diagonal(p);
}

template <typename Parallelotope>
typename ParallelotopeAlgorithm<Parallelotope>::Diagonals parallelotope_diagonals(const Parallelotope& p)
{
        return ParallelotopeAlgorithm<Parallelotope>::diagonals(p);
}

template <typename Parallelotope>
typename ParallelotopeAlgorithm<Parallelotope>::Vertices parallelotope_vertices(const Parallelotope& p)
{
        return ParallelotopeAlgorithm<Parallelotope>::vertices(p);
}

template <typename Parallelotope>
typename ParallelotopeAlgorithm<Parallelotope>::VertexRidges parallelotope_vertex_ridges(const Parallelotope& p)
{
        return ParallelotopeAlgorithm<Parallelotope>::vertex_ridges(p);
}
