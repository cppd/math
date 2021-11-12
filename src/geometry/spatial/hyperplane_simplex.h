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

#include <src/numerical/complement.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <array>
#include <optional>
#include <utility>

namespace ns::geometry
{
template <std::size_t N, typename T>
class HyperplaneSimplex final
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

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

        T barycentric_coordinate(const Vector<N, T>& point, const unsigned i) const
        {
                // The distance from the ridge to the point is the barycentric coordinate.
                return dot(point, planes_[i].n) - planes_[i].d;
        }

        Vector<N, T> barycentric_coordinates(const Vector<N, T>& point) const
        {
                Vector<N, T> result;
                result[N - 1] = 1;
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        result[i] = barycentric_coordinate(point, i);
                        result[N - 1] -= result[i];
                }
                return result;
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

                        const T distance = dot(vertices[i], planes_[i].n) - planes_[i].d;
                        planes_[i].n /= distance;
                        planes_[i].d /= distance;
                }
        }

        // N constraints b + a * x >= 0
        // one constraint b + a * x = 0
        // normal and vertices are the same as in set_data function
        Constraints<N, T, N, 1> constraints(const Vector<N, T>& normal, const std::array<Vector<N, T>, N>& vertices)
                const
        {
                Constraints<N, T, N, 1> result;

                // Planes n * x - d have vectors n directed inward.
                // Points are inside if n * x - d >= 0 or -d + n * x >= 0.

                // There are already N - 1 planes passing through vertex N - 1
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        const T len = planes_[i].n.norm();
                        result.c[i].a = planes_[i].n / len;
                        result.c[i].b = -planes_[i].d / len;
                }

                {
                        // Create a plane that do not pass through vertex N - 1
                        // Based on set_data function.

                        std::array<Vector<N, T>, N - 1> vectors;
                        for (std::size_t i = 0; i < N - 2; ++i)
                        {
                                vectors[i] = vertices[i + 1] - vertices[0];
                        }
                        vectors[N - 2] = normal;

                        const Vector<N, T> n = numerical::orthogonal_complement(vectors).normalized();
                        const T d = dot(vertices[0], n);

                        // normal must be directed to vertex N - 1
                        const bool to_vertex = dot(vertices[N - 1], n) - d >= 0;
                        result.c[N - 1].a = to_vertex ? n : -n;
                        result.c[N - 1].b = to_vertex ? -d : d;
                }

                {
                        // based on the simplex plane equation n * x - d = 0
                        const T d = dot(vertices[0], normal);
                        result.c_eq[0].a = normal;
                        result.c_eq[0].b = -d;
                }

                return result;
        }

        std::optional<T> intersect(const Ray<N, T>& ray, const Vector<N, T>& plane_n, const T& plane_d) const
        {
                const std::optional<T> t = hyperplane_intersect(ray, plane_n, plane_d);
                if (!t)
                {
                        return std::nullopt;
                }

                const Vector<N, T> point = ray.point(*t);

                Vector<N - 1, T> bc;
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        bc[i] = barycentric_coordinate(point, i);
                        if (!(bc[i] > 0 && bc[i] < 1))
                        {
                                return std::nullopt;
                        }
                }

                T sum = bc[0];
                for (std::size_t i = 1; i < N - 1; ++i)
                {
                        sum += bc[i];
                }
                if (sum < 1)
                {
                        return t;
                }
                return std::nullopt;
        }

        template <std::size_t M>
        Vector<M, T> interpolate(const Vector<N, T>& point, const std::array<Vector<M, T>, N>& data) const
        {
                const Vector<N, T> bc = barycentric_coordinates(point);

                Vector<M, T> result = bc[0] * data[0];
                for (std::size_t i = 1; i < N; ++i)
                {
                        result.multiply_add(bc[i], data[i]);
                }
                return result;
        }
};
}
