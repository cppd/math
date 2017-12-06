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

/*
 R. Stuart Ferguson.
 Practical Algorithms For 3D Computer Graphics, Second Edition.
 CRC Press, 2014.

 В частности, раздел 5.3.4 Octree decomposition.
*/

#include "intersection.h"

#include <type_traits>

namespace
{
template <typename Shape>
bool shape_intersected_by_edge(const Shape& shape, const vec3& org, const vec3& edge)
{
        ray3 r = ray3(org, edge);
        double alpha;
        return shape.intersect(r, &alpha) && (alpha * alpha < dot(edge, edge));
}

template <typename Shape, typename... Vector3>
bool shape_intersected_by_edges(const Shape& shape, const vec3& org, const Vector3&... edge)
{
        static_assert((std::is_same_v<vec3, Vector3> && ...));
        return (shape_intersected_by_edge(shape, org, edge) || ...);
}

template <typename Shape>
bool triangle_intersects_shape(const IntersectionTriangle& t, const Shape& shape)
{
        return shape_intersected_by_edge(shape, t.v0(), t.v1() - t.v0()) ||
               shape_intersected_by_edge(shape, t.v1(), t.v2() - t.v1()) ||
               shape_intersected_by_edge(shape, t.v2(), t.v0() - t.v2());
}

template <typename Shape>
bool parallelepiped_intersects_shape(const IntersectionParallelotope<3, double>& p, const Shape& shape)
{
        return shape_intersected_by_edges(shape, p.org(), p.e(0), p.e(1), p.e(2)) ||
               shape_intersected_by_edges(shape, p.org() + p.e(1) + p.e(2), p.e(0), -p.e(1), -p.e(2)) ||
               shape_intersected_by_edges(shape, p.org() + p.e(0) + p.e(2), -p.e(0), p.e(1), -p.e(2)) ||
               shape_intersected_by_edges(shape, p.org() + p.e(0) + p.e(1), -p.e(0), -p.e(1), p.e(2));
}

template <typename Shape>
bool triangle_inside_shape(const IntersectionTriangle& t, const Shape& shape)
{
        return shape.inside(t.v0()) || shape.inside(t.v1()) || shape.inside(t.v2());
}

template <typename Shape>
bool parallelepiped_inside_shape(const IntersectionParallelotope<3, double>& p, const Shape& shape)
{
        // clang-format off
        return
                shape.inside(p.org()) ||
                shape.inside(p.org() + p.e(0)) ||
                shape.inside(p.org() + p.e(1)) ||
                shape.inside(p.org() + p.e(2)) ||
                shape.inside(p.org() + p.e(0) + p.e(1)) ||
                shape.inside(p.org() + p.e(0) + p.e(2)) ||
                shape.inside(p.org() + p.e(1) + p.e(2)) ||
                shape.inside(p.org() + p.e(0) + p.e(1) + p.e(2));
        // clang-format on
}
}

// Треугольник и параллелепипед пересекаются, если выполняется любое из условий:
//   1) какая-нибудь вершина треугольника находится внутри параллелепипеда,
//   2) какое-нибудь одно из 3 ребер треугольника пересекает параллелепипед,
//   3) какое-нибудь одно из 12 рёбер параллелепипеда пересекает треугольник.
bool shape_intersection(const IntersectionParallelotope<3, double>& p, const IntersectionTriangle& t)
{
        // clang-format off
        return
                triangle_inside_shape(t, p) ||
                triangle_intersects_shape(t, p) ||
                parallelepiped_intersects_shape(p, t);
        // clang-format on
}

// Параллелепипеды пересекаются, если выполняется любое из условий:
//   1) какая-нибудь вершина какого-нибудь параллелепипеда находится внутри другого параллелепипеда,
//   2) какое-нибудь одно из 12 ребер какого-нибудь параллелепипеда пересекает другой параллелепипед.
bool shape_intersection(const IntersectionParallelotope<3, double>& p1, const IntersectionParallelotope<3, double>& p2)
{
        // clang-format off
        return
                parallelepiped_inside_shape(p1, p2) ||
                parallelepiped_inside_shape(p2, p1) ||
                parallelepiped_intersects_shape(p1, p2) ||
                parallelepiped_intersects_shape(p2, p1);
        // clang-format on
}
