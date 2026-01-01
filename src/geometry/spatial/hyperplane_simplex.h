/*
Copyright (C) 2017-2026 Topological Manifold

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
#include <src/com/print.h>
#include <src/numerical/complement.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <optional>

namespace ns::geometry::spatial
{
template <std::size_t N, typename T>
class HyperplaneSimplex final
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        // (N - 1)-dimensional simplex plane.
        Hyperplane<N, T> plane_;

        // (N - 1)-dimensional planes orthogonal to the simplex
        // and passing through its ridges except for one ridge.
        std::array<Hyperplane<N, T>, N - 1> planes_;

        [[nodiscard]] T barycentric_coordinate(const numerical::Vector<N, T>& point, const std::size_t i) const
        {
                return planes_[i].distance(point);
        }

        [[nodiscard]] numerical::Vector<N, T> barycentric_coordinates(const numerical::Vector<N, T>& point) const
        {
                numerical::Vector<N, T> res;
                res[N - 1] = 1;
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        res[i] = barycentric_coordinate(point, i);
                        res[N - 1] -= res[i];
                }
                return res;
        }

public:
        static T intersection_cost();

        HyperplaneSimplex()
        {
        }

        explicit HyperplaneSimplex(const std::array<numerical::Vector<N, T>, N>& vertices)
        {
                set(vertices);
        }

        void set(const std::array<numerical::Vector<N, T>, N>& vertices)
        {
                std::array<numerical::Vector<N, T>, N - 1> vectors;
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        vectors[i] = vertices[i] - vertices[N - 1];
                }

                plane_.n = numerical::orthogonal_complement(vectors).normalized();
                if (!is_finite(plane_.n))
                {
                        error("Hyperplane simplex normal " + to_string(plane_.n) + " is not finite, vertices "
                              + to_string(vertices));
                }
                plane_.d = dot(plane_.n, vertices[N - 1]);

                // create N - 1 planes that pass through vertex N - 1,
                // through simples ridges, and that are orthogonal to the simplex.
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        std::swap(plane_.n, vectors[i]);
                        planes_[i].n = numerical::orthogonal_complement(vectors);
                        std::swap(plane_.n, vectors[i]);

                        // dot(p - org, normal) = dot(p, normal) - dot(org, normal) = dot(p, normal) - d
                        // org = vertices[N - 1]
                        planes_[i].d = dot(vertices[N - 1], planes_[i].n);

                        const T distance = planes_[i].distance(vertices[i]);
                        planes_[i].n /= distance;
                        planes_[i].d /= distance;
                }
        }

        void reverse_normal()
        {
                plane_.reverse_normal();
        }

        [[nodiscard]] const numerical::Vector<N, T>& normal() const
        {
                return plane_.n;
        }

        [[nodiscard]] numerical::Vector<N, T> project(const numerical::Vector<N, T>& point) const
        {
                return plane_.project(point);
        }

        // N constraints b + a * x >= 0
        // one constraint b + a * x = 0
        // vertices must be the same as in the set function
        [[nodiscard]] Constraints<N, T, N, 1> constraints(const std::array<numerical::Vector<N, T>, N>& vertices) const
        {
                Constraints<N, T, N, 1> res;

                // Planes n * x - d have vectors n directed inward.
                // Points are inside if n * x - d >= 0 or -d + n * x >= 0.

                // There are already N - 1 planes passing through vertex N - 1
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        const T len = planes_[i].n.norm();
                        res.c[i].a = planes_[i].n / len;
                        res.c[i].b = -planes_[i].d / len;
                }

                {
                        // Create a plane that do not pass through vertex N - 1
                        std::array<numerical::Vector<N, T>, N - 1> vectors;
                        for (std::size_t i = 0; i < N - 2; ++i)
                        {
                                vectors[i] = vertices[i + 1] - vertices[0];
                        }
                        vectors[N - 2] = plane_.n;
                        const numerical::Vector<N, T> n = numerical::orthogonal_complement(vectors).normalized();
                        const T d = dot(vertices[0], n);

                        // normal must be directed to vertex N - 1
                        const bool to_vertex = dot(vertices[N - 1], n) - d >= 0;
                        res.c[N - 1].a = to_vertex ? n : -n;
                        res.c[N - 1].b = to_vertex ? -d : d;
                }

                res.c_eq[0].a = plane_.n;
                res.c_eq[0].b = -plane_.d;

                return res;
        }

        [[nodiscard]] std::optional<T> intersect(const numerical::Ray<N, T>& ray) const
        {
                const T t = plane_.intersect(ray);
                if (!(t > 0))
                {
                        return std::nullopt;
                }

                const numerical::Vector<N, T> point = ray.point(t);

                numerical::Vector<N - 1, T> bc;
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
        [[nodiscard]] numerical::Vector<M, T> interpolate(
                const numerical::Vector<N, T>& point,
                const std::array<numerical::Vector<M, T>, N>& data) const
        {
                const numerical::Vector<N, T> bc = barycentric_coordinates(point);

                numerical::Vector<M, T> res = bc[0] * data[0];
                for (std::size_t i = 1; i < N; ++i)
                {
                        res.multiply_add(bc[i], data[i]);
                }
                return res;
        }
};
}
