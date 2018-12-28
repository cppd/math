/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "com/math.h"
#include "com/types.h"
#include "com/vec.h"
#include "numerical/quadratic.h"

namespace cocone_implementation
{
// Константа алгоритмов Cocone, равна cos(3 * PI / 8)
template <typename T>
constexpr T COS_OF_AN_OPENING_ANGLE_WITH_THE_AXIS =
        0.3826834323650897717284599840303988667613445624856270414338006356275460339600896922370137853422835471L;
}

// Условия пересечения ребра ячейки Вороного, соответствующего грани, с cocone вершины.
// Параметры - косинусы между перпендикуляром к вершине и двумя вершинами ребра ячейки Вороного.
template <typename T>
bool voronoi_edge_intersects_cocone(T cos_n_a, T cos_n_b)
{
        static_assert(is_native_floating_point<T>);

        constexpr T cos_cocone = cocone_implementation::COS_OF_AN_OPENING_ANGLE_WITH_THE_AXIS<T>;

        if (any_abs(cos_n_a) < cos_cocone || any_abs(cos_n_b) < cos_cocone)
        {
                return true;
        }
        if (cos_n_a < 0 && cos_n_b > 0)
        {
                return true;
        }
        if (cos_n_a > 0 && cos_n_b < 0)
        {
                return true;
        }
        return false;
}

template <typename... T>
bool cocone_inside_or_equal(T... cos_n_p)
{
        static_assert((is_native_floating_point<T> && ...));

        return ((any_abs(cos_n_p) <= cocone_implementation::COS_OF_AN_OPENING_ANGLE_WITH_THE_AXIS<T>)&&...);
}

/*
Пересечение отрезка AB с двойным конусом с заданной осью и заданным углом между осью и конусом.

Вектор от центра конуса к точке A равен PA, вектор от центра конуса к точке B равен PB.
Вектор от центра конуса к точке пересечения AB с конусом равен PI = PA + t × (PB - PA),
где 0 <= t <= 1.

Надо найти такие t, при которых косинус между PI и единичным вектором оси конуса равен заданному
косинусу со знаком плюс или минус:
        normalize(PA + t × (PB - PA))⋅N = ±cos(alpha).
Из двух t надо выбрать одно такое, при котором вектор PI максимален.

Обозначаем a = PA, b = PB, n = N:
        ((a + t×(b-a))/norm(a+t×(b-a)))⋅n = ±cos(alpha),
        ((a⋅n + t×(b-a)⋅n))/norm(a+t×(b-a)) = ±cos(alpha),
        ((a⋅n + t×(b-a)⋅n))²/(a+t×(b-a))² = cos²(alpha).

Далее раскрываем квадраты и группируем по степеням t.
Получается квадратное уравнение относительно t:
          t² × ((n⋅(b-a))² - cos²(alpha)×(b-a)²)
        + t¹ × 2 × ((a⋅n)(n⋅(b-a)) - a⋅(b-a)×cos²(alpha))
        + t⁰ × ((a⋅n)² - a²×cos²(alpha))
          = 0
*/
template <size_t N, typename T>
bool intersect_cocone(const Vector<N, T>& normalized_cone_axis, const Vector<N, T>& from_apex_to_point_a,
                      const Vector<N, T>& from_point_a_to_point_b, T* distance)
{
        static_assert(is_native_floating_point<T>);

        const Vector<N, T>& vec_a = from_apex_to_point_a;
        const Vector<N, T>& vec_ab = from_point_a_to_point_b;
        const Vector<N, T>& vec_norm = normalized_cone_axis;

        T n_ab = dot(vec_norm, vec_ab);
        T a_n = dot(vec_a, vec_norm);
        T square_a = dot(vec_a, vec_a);
        T square_ab = dot(vec_ab, vec_ab);
        T a_ab = dot(vec_a, vec_ab);
        T square_cos = square(cocone_implementation::COS_OF_AN_OPENING_ANGLE_WITH_THE_AXIS<T>);

        // коэффициенты квадратного уравнения ax² + bx + c = 0
        T a = square(n_ab) - square_cos * square_ab;
        T b = 2 * (a_n * n_ab - a_ab * square_cos);
        T c = square(a_n) - square_a * square_cos;

        T t1, t2;
        if (!numerical::quadratic_equation(a, b, c, &t1, &t2))
        {
                return false;
        }

        bool t1_ok = t1 >= 0 && t1 <= limits<T>::max();
        bool t2_ok = t2 >= 0 && t2 <= limits<T>::max();

        if (!t1_ok && !t2_ok)
        {
                return false;
        }

        Vector<N, T> from_apex_to_intersection_point;

        if (t1_ok && !t2_ok)
        {
                from_apex_to_intersection_point = vec_a + t1 * vec_ab;
                *distance = length(from_apex_to_intersection_point);
                return true;
        }
        if (!t1_ok && t2_ok)
        {
                from_apex_to_intersection_point = vec_a + t2 * vec_ab;
                *distance = length(from_apex_to_intersection_point);
                return true;
        }

        Vector<N, T> pi_1 = vec_a + t1 * vec_ab;
        Vector<N, T> pi_2 = vec_a + t2 * vec_ab;
        T dot_1 = dot(pi_1, pi_1);
        T dot_2 = dot(pi_2, pi_2);

        if (dot_1 > dot_2)
        {
                from_apex_to_intersection_point = pi_1;
                *distance = any_sqrt(dot_1);
        }
        else
        {
                from_apex_to_intersection_point = pi_2;
                *distance = any_sqrt(dot_2);
        }

        return true;
}
