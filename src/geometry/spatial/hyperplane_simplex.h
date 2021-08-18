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

/*
Samuel R. Buss.
3D Computer Graphics. A Mathematical Introduction with OpenGL.
Cambridge University Press, 2003.
*/

#pragma once

#include "constraint.h"
#include "hyperplane.h"

#include <src/com/error.h>
#include <src/com/type/trait.h>
#include <src/numerical/complement.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace ns::geometry
{
template <std::size_t N, typename T>
class HyperplaneSimplex final
{
        static_assert(N >= 2);
        static_assert(is_floating_point<T>);

        // (N-1)-dimensional planes
        // They are orthogonal to (N-1)-simplex and passing
        // through its ridges except for one ridge.
        // Only planes are stored. Simplex normal and vertices are passed
        // in function parameters so as not to duplicate data.
        struct Plane
        {
                Vector<N, T> n;
                T d;
        };
        std::array<Plane, N - 1> planes_;

        static T last_coordinate(const Vector<N - 1, T>& coordinates)
        {
                T r = 1;
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        r -= coordinates[i];
                }
                return r;
        }

public:
        void set_data(Vector<N, T> normal, const std::array<Vector<N, T>, N>& vertices)
        {
                std::array<Vector<N, T>, N - 1> vectors;
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        vectors[i] = vertices[i] - vertices[N - 1];
                }

                // normal must be equal to orthogonal_complement(vectors)

                // create N - 1 planes that pass through vertex N - 1,
                // through simples ridges, and that are orthogonal to the simplex.
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        std::swap(normal, vectors[i]);
                        planes_[i].n = numerical::orthogonal_complement(vectors);
                        std::swap(normal, vectors[i]);

                        // dot(p - org, normal) = dot(p, normal) - dot(org, normal) = dot(p, normal) - d
                        // org = vertices[N - 1]
                        planes_[i].d = dot(vertices[N - 1], planes_[i].n);

                        T distance = dot(vertices[i], planes_[i].n) - planes_[i].d;
                        planes_[i].n /= distance;
                        planes_[i].d /= distance;
                }
        }

        // N constraints b + a * x >= 0
        // one constraint b + a * x = 0
        // normal and vertices are the same as in set_data function
        Constraints<N, T, N, 1> constraints(Vector<N, T> normal, const std::array<Vector<N, T>, N>& vertices) const
        {
                Constraints<N, T, N, 1> result;

                // Planes n * x - d have vectors n directed inward.
                // Points are inside if n * x - d >= 0 or -d + n * x >= 0.

                // There are already N - 1 planes passing through vertex N - 1
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        T len = planes_[i].n.norm();
                        result.c[i].a = planes_[i].n / len;
                        result.c[i].b = -planes_[i].d / len;
                }

                //

                // Create a plane that do not pass through vertex N - 1
                // Based on set_data function.

                std::array<Vector<N, T>, N - 1> vectors;
                for (std::size_t i = 0; i < N - 2; ++i)
                {
                        vectors[i] = vertices[i + 1] - vertices[0];
                }

                vectors[N - 2] = normal;
                Vector<N, T> n = numerical::orthogonal_complement(vectors).normalized();
                T d = dot(vertices[0], n);

                // normal must be directed to vertex N - 1
                bool to_vertex = dot(vertices[N - 1], n) - d >= 0;
                result.c[N - 1].a = to_vertex ? n : -n;
                result.c[N - 1].b = to_vertex ? -d : d;

                //

                // based on the simplex plane equation n * x - d = 0
                d = dot(vertices[0], normal);
                result.c_eq[0].a = normal;
                result.c_eq[0].b = -d;

                return result;
        }

        T barycentric_coordinate(const Vector<N, T>& point, unsigned i) const
        {
                ASSERT(i < N - 1);
                // The distance from the ridge to the point is the barycentric coordinate.
                return dot(point, planes_[i].n) - planes_[i].d;
        }

        Vector<N, T> barycentric_coordinates(const Vector<N, T>& point) const
        {
                Vector<N, T> coords;
                coords[N - 1] = 1;
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        coords[i] = barycentric_coordinate(point, i);
                        coords[N - 1] -= coords[i];
                }
                return coords;
        }

        std::optional<T> intersect(const Ray<N, T>& ray, const Vector<N, T>& any_vertex, const Vector<N, T>& normal)
                const
        {
                std::optional<T> t = hyperplane_intersect(ray, any_vertex, normal);
                if (!t)
                {
                        return std::nullopt;
                }

                Vector<N, T> intersection_point = ray.point(*t);

                Vector<N - 1, T> coordinates;

                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        coordinates[i] = barycentric_coordinate(intersection_point, i);
                        if (coordinates[i] <= 0 || coordinates[i] >= 1)
                        {
                                return std::nullopt;
                        }
                }

                if (last_coordinate(coordinates) > 0)
                {
                        return t;
                }
                return std::nullopt;
        }

        template <typename InterpolationType>
        InterpolationType interpolate(const Vector<N, T>& point, const std::array<InterpolationType, N>& n) const
        {
                Vector<N, T> bc = barycentric_coordinates(point);

                InterpolationType result = bc[0] * n[0];

                for (std::size_t i = 1; i < N; ++i)
                {
                        result += bc[i] * n[i];
                }

                return result;
        }
};
}
