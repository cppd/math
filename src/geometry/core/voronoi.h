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

/*
Описание есть, например, в книге

Satyan L. Devadoss, Joseph O’Rourke.
Discrete and computational geometry.
Princeton University Press, 2011.

  Вершина Вороного, соответствующая объекту Делоне из n вершин, является также
центром (n-1)-мерной сферы, проходящей через все n вершин объекта Делоне.
  Эта вершина может быть найдена двумя способами, однаковыми математически,
но численно они могут давать разные результаты, особенно с параболоидом
с его здесь ненужным дополнительным измерением и квадратами чисел, быстро
уменьшающими точность расчётов.
  1)
      Пересечение n-1 подпространств, перпендикулярных n-1 одномерным векторам
    вершина-вершина и проходящим через середины этих векторов.
    Для определения этой точки надо решить систему линейных уравнений для всех n
    точек A1, ..., An симплекса
    2*(A2(1)-A1(1)) ... 2*(A2(n-1) - A1(n-1)) = dot(A2, A2) - dot(A1, A1)
    2*(A3(1)-A1(1)) ... 2*(A3(n-1) - A1(n-1)) = dot(A3, A3) - dot(A1, A1)
    ...
    2*(An(1)-A1(1)) ... 2*(An(n-1) - A1(n-1)) = dot(An, An) - dot(A1, A1)

  2)
      Пересечение касательных подпространств к параболоиду в всех n вершинах симплекса.
    Уравнение касательного (n-1)-мерного подпространства к n-мерному параболоиду
    в точке A с координатами A(1), ..., A(n):
            2*A(1)*x(1) + ... + 2*A(n-1)*x(n-1) - x(n) = A(n).
    Для определения этой точки надо решить систему линейных уравнений для всех n
    точек A1, ..., An симплекса, затем убрать последнюю координату в решении.
    2*A1(1) ... 2*A1(n-1) - 1 = A1(n)
    2*A2(1) ... 2*A2(n-1) - 1 = A2(n)
    ...
    2*An(1) ... 2*An(n-1) - 1 = An(n)
*/

#pragma once

#include "com/matrix.h"
#include "com/vec.h"
#include "numerical/linear.h"

#include <array>
#include <vector>

#if 0
#include <Eigen/Core>
#include <Eigen/LU>
#endif

// Для вершин объекта Делоне найти вершину Вороного, соответствующую этому объекту Делоне
template <size_t N, typename T>
Vector<N, T> compute_voronoi_vertex(const std::vector<Vector<N, T>>& points, const std::array<int, N + 1>& vertices)
{
#if 1
        // Матрица системы уравнений
        Matrix<N, N, T> a;
        // Правая часть системы уравнений
        Vector<N, T> b;

        Vector<N, T> p0 = points[vertices[0]];
        T dot0 = dot(p0, p0);

        for (unsigned row = 0; row < N; ++row)
        {
                Vector<N, T> pn = points[vertices[row + 1]];
                Vector<N, T> pn_minus_p0 = pn - p0;
                for (unsigned col = 0; col < N; ++col)
                {
                        a[row][col] = 2 * pn_minus_p0[col];
                }
                b[row] = dot(pn, pn) - dot0;
        }

        Vector<N, T> voronoi_vertex = numerical::solve(std::move(a), b);

        ASSERT(is_finite(voronoi_vertex));

        return voronoi_vertex;
#else

#if 1
        // Матрица системы уравнений
        Eigen::Matrix<T, N, N> a;
        // Правая часть системы уравнений
        Eigen::Matrix<T, N, 1> b;

        Vector<N, T> p0 = points[vertices[0]];
        T dot0 = dot(p0, p0);

        for (unsigned row = 0; row < N; ++row)
        {
                Vector<N, T> pn = points[vertices[row + 1]];
                Vector<N, T> pn_minus_p0 = pn - p0;
                for (unsigned col = 0; col < N; ++col)
                {
                        a.coeffRef(row, col) = 2 * pn_minus_p0[col];
                }
                b.coeffRef(row, 0) = dot(pn, pn) - dot0;
        }

        // Решение системы уравнений
        Eigen::Matrix<T, N, 1> s = a.fullPivLu().solve(b);

        Vector<N, T> voronoi_vertex;
        for (unsigned n = 0; n < N; ++n)
        {
                voronoi_vertex[n] = s.coeff(n);
        }

        return voronoi_vertex;
#else
        // Матрица системы уравнений
        Eigen::Matrix<T, N + 1, N + 1> a;
        // Правая часть системы уравнений
        Eigen::Matrix<T, N + 1, 1> b;

        for (unsigned row = 0; row < N + 1; ++row)
        {
                Vector<N, T> p = points[vertices[row]];
                T paraboloid_f = 0;
                for (unsigned col = 0; col < N; ++col)
                {
                        a.coeffRef(row, col) = 2 * p[col];
                        paraboloid_f += square(p[col]);
                }
                a.coeffRef(row, N) = -1;
                b.coeffRef(row, 0) = paraboloid_f;
        }

        // Решение системы уравнений
        Eigen::Matrix<T, N + 1, 1> s = a.fullPivLu().solve(b);

        //Проецирование в исходное пространство размерности N
        Vector<N, T> voronoi_vertex;
        for (unsigned n = 0; n < N; ++n)
        {
                voronoi_vertex[n] = s.coeff(n);
        }

        return voronoi_vertex;
#endif
#endif
}
