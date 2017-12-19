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

#pragma once

#include "com/math.h"
#include "com/ray.h"
#include "com/vec.h"

#include <array>
#include <type_traits>

template <typename DerivedSimplex>
class IntersectionSimplex
{
protected:
        IntersectionSimplex() = default;
        IntersectionSimplex(const IntersectionSimplex&) = default;
        IntersectionSimplex(IntersectionSimplex&&) = default;
        IntersectionSimplex& operator=(const IntersectionSimplex&) = default;
        IntersectionSimplex& operator=(IntersectionSimplex&&) = default;
        ~IntersectionSimplex() = default;
};

template <typename DerivedParallelotope>
class IntersectionParallelotope
{
protected:
        IntersectionParallelotope() = default;
        IntersectionParallelotope(const IntersectionParallelotope&) = default;
        IntersectionParallelotope(IntersectionParallelotope&&) = default;
        IntersectionParallelotope& operator=(const IntersectionParallelotope&) = default;
        IntersectionParallelotope& operator=(IntersectionParallelotope&&) = default;
        ~IntersectionParallelotope() = default;
};

namespace ShapeIntersectionImplementation
{
template <size_t N, typename T, typename Shape>
bool line_segment_intersects_shape(const Vector<N, T>& org, const Vector<N, T>& direction, const Shape& shape)
{
        Ray<N, T> r(org, direction);
        T alpha;
        return shape.intersect(r, &alpha) && (square(alpha) < dot(direction, direction));
}

template <typename Parallelotope, typename Vertices>
bool parallelotope_contains_vertices(const Parallelotope& parallelotope, const Vertices& vertices)
{
        constexpr size_t N = Parallelotope::DIMENSION;
        using T = typename Parallelotope::DataType;

        for (const Vector<N, T>& v : vertices)
        {
                if (parallelotope.inside(v))
                {
                        return true;
                }
        }

        return false;
}

template <typename Parallelotope, typename Shape>
bool parallelotope_intersects_shape(const Parallelotope& parallelotope, const Shape& shape)
{
        constexpr size_t N = Parallelotope::DIMENSION;
        using T = typename Parallelotope::DataType;

        static_assert(std::remove_reference_t<decltype(parallelotope.vertex_ridges())>().size() == (1 << (N - 1)) * N);

        for (const std::array<Vector<N, T>, 2>& ridge : parallelotope.vertex_ridges())
        {
                if (line_segment_intersects_shape(ridge[0], ridge[1], shape))
                {
                        return true;
                }
        }

        return false;
}

template <typename Parallelotope, typename Vertices>
bool parallelotope_is_intersected_by_simplex(const Parallelotope& parallelotope, const Vertices& vertices)
{
        static_assert(Parallelotope::DIMENSION == std::remove_reference_t<decltype(vertices)>().size());

        for (unsigned i = 0; i < vertices.size() - 1; ++i)
        {
                for (unsigned j = i + 1; j < vertices.size(); ++j)
                {
                        if (line_segment_intersects_shape(vertices[i], vertices[j] - vertices[i], parallelotope))
                        {
                                return true;
                        }
                }
        }

        return false;
}
}

// (n-1)-симплекс и n-параллелотоп пересекаются, если выполняется любое из условий:
//   1) какая-нибудь вершина симплекса находится внутри параллелотопа,
//   2) какое-нибудь одно из линейных рёбер симплекса пересекает параллелотоп,
//   3) какое-нибудь одно из линейных рёбер параллелотопа пересекает симплекс.
template <typename Parallelotope, typename Simplex>
bool shape_intersection(const IntersectionParallelotope<Parallelotope>& p, const IntersectionSimplex<Simplex>& s)
{
        namespace Impl = ShapeIntersectionImplementation;

        constexpr size_t N = Parallelotope::DIMENSION;
        using T = typename Parallelotope::DataType;

        const Parallelotope& parallelotope = static_cast<const Parallelotope&>(p);
        const Simplex& simplex = static_cast<const Simplex&>(s);

        std::array<Vector<N, T>, N> simplex_vertices = simplex.vertices();

        return Impl::parallelotope_contains_vertices(parallelotope, simplex_vertices) ||
               Impl::parallelotope_is_intersected_by_simplex(parallelotope, simplex_vertices) ||
               Impl::parallelotope_intersects_shape(parallelotope, simplex);
}

// Два n-параллелотопа пересекаются, если выполняется любое из условий:
//   1) какая-нибудь вершина какого-нибудь одного параллелотопа находится внутри другого параллелотопа,
//   2) какое-нибудь одно из линейных рёбер одного параллелотопа пересекает другой параллелотоп.
template <typename Parallelotope1, typename Parallelotope2>
bool shape_intersection(const IntersectionParallelotope<Parallelotope1>& a, const IntersectionParallelotope<Parallelotope2>& b)
{
        namespace Impl = ShapeIntersectionImplementation;

        const Parallelotope1& parallelotope_1 = static_cast<const Parallelotope1&>(a);
        const Parallelotope2& parallelotope_2 = static_cast<const Parallelotope2&>(b);

        return Impl::parallelotope_contains_vertices(parallelotope_1, parallelotope_2.vertices()) ||
               Impl::parallelotope_intersects_shape(parallelotope_1, parallelotope_2) ||
               Impl::parallelotope_contains_vertices(parallelotope_2, parallelotope_1.vertices()) ||
               Impl::parallelotope_intersects_shape(parallelotope_2, parallelotope_1);
}
