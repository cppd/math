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
Matt Pharr, Wenzel Jakob, Greg Humphreys.
Physically Based Rendering. From theory to implementation. Third edition.
Elsevier, 2017.

2.6 Bounding boxes
3.1.2 Ray–bounds intersections
*/

#pragma once

#include <src/com/type/limit.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <array>
#include <optional>

namespace ns::geometry
{
template <std::size_t N, typename T>
class BoundingBox final
{
        static_assert(N >= 1);
        static_assert(std::is_floating_point_v<T>);

        template <std::size_t M>
        static constexpr T volume(const Vector<N, T>& d)
        {
                static_assert(M <= N);
                static_assert(M >= 1);
                if constexpr (M == 1)
                {
                        return d[0];
                }
                else if constexpr (M == 2)
                {
                        return d[0] * d[1];
                }
                else if constexpr (M == 3)
                {
                        return d[0] * d[1] * d[2];
                }
                else
                {
                        return d[M - 1] * volume<M - 1>(d);
                }
        }

        template <std::size_t M>
        static constexpr T surface(const Vector<N, T>& d)
        {
                static_assert(M <= N);
                static_assert(M >= 2);
                if constexpr (M == 2)
                {
                        return d[0] + d[1];
                }
                else if constexpr (M == 3)
                {
                        return d[0] * d[1] + d[2] * (d[0] + d[1]);
                }
                else
                {
                        return volume<M - 1>(d) + d[M - 1] * surface<M - 1>(d);
                }
        }

        std::array<Vector<N, T>, 2> bounds_;

        static_assert(Limits<T>::is_iec559());

        // std::min and std::max return the first argument if the second argument is NaN
        // if direction == 0 then alpha1 and alpha2 have values
        //   (infinity, infinity) -> near=infinity, far=far -> return false
        //   (NaN, infinity) -> near=near, far=far -> continue
        //   (-infinity, infinity) -> near=near, far=far -> continue
        //   (-infinity, NaN) -> near=near, far=far -> continue
        //   (-infinity, -infinity) -> near=near, far=-infinity -> return false

        bool intersect(const Ray<N, T>& ray, T* const first, T* const second) const
        {
                T near = 0;
                T far = Limits<T>::max();
                for (std::size_t i = 0; i < N; ++i)
                {
                        const T dir = ray.dir()[i];
                        const T d = ray.org()[i];
                        const T r = 1 / dir;
                        const T alpha1 = (bounds_[0][i] - d) * r;
                        const T alpha2 = (bounds_[1][i] - d) * r;
                        if (dir >= 0)
                        {
                                near = std::max(near, alpha1);
                                far = std::min(far, alpha2);
                        }
                        else
                        {
                                near = std::max(near, alpha2);
                                far = std::min(far, alpha1);
                        }
                        if (far < near)
                        {
                                return false;
                        }
                }
                *first = near;
                *second = far;
                return true;
        }

        bool intersect(
                const Vector<N, T>& org,
                const Vector<N, T>& dir_reciprocal,
                const Vector<N, bool>& dir_negative,
                T* const first,
                T* const second) const
        {
                T near = 0;
                T far = Limits<T>::max();
                for (std::size_t i = 0; i < N; ++i)
                {
                        const T d = org[i];
                        const T r = dir_reciprocal[i];
                        const T alpha1 = (bounds_[dir_negative[i]][i] - d) * r;
                        const T alpha2 = (bounds_[!dir_negative[i]][i] - d) * r;
                        near = std::max(near, alpha1);
                        far = std::min(far, alpha2);
                        if (far < near)
                        {
                                return false;
                        }
                }
                *first = near;
                *second = far;
                return true;
        }

public:
        constexpr BoundingBox(const Vector<N, T>& p1, const Vector<N, T>& p2)
                : bounds_{::ns::min(p1, p2), ::ns::max(p1, p2)}
        {
        }

        explicit constexpr BoundingBox(const Vector<N, T>& p) : bounds_{p, p}
        {
        }

        template <std::size_t SIZE>
        explicit constexpr BoundingBox(const std::array<Vector<N, T>, SIZE>& points) : BoundingBox(points[0])
        {
                static_assert(SIZE > 0);
                for (std::size_t i = 1; i < SIZE; ++i)
                {
                        merge(points[i]);
                }
        }

        [[nodiscard]] constexpr const Vector<N, T>& min() const
        {
                return bounds_[0];
        }

        [[nodiscard]] constexpr const Vector<N, T>& max() const
        {
                return bounds_[1];
        }

        [[nodiscard]] constexpr Vector<N, T> diagonal() const
        {
                return bounds_[1] - bounds_[0];
        }

        [[nodiscard]] constexpr Vector<N, T> center() const
        {
                return T(0.5) * (bounds_[0] + bounds_[1]);
        }

        [[nodiscard]] constexpr T volume() const
        {
                return volume<N>(diagonal());
        }

        [[nodiscard]] constexpr T surface() const requires(N >= 2)
        {
                return surface<N>(diagonal());
        }

        [[nodiscard]] std::optional<T> intersect(const Ray<N, T>& r) const
        {
                T first;
                T second;
                if (intersect(r, &first, &second))
                {
                        return (first > 0) ? first : second;
                }
                return std::nullopt;
        }

        [[nodiscard]] std::optional<T> intersect(
                const Vector<N, T>& org,
                const Vector<N, T>& dir_reciprocal,
                const Vector<N, bool>& dir_negative) const
        {
                T first;
                T second;
                if (intersect(org, dir_reciprocal, dir_negative, &first, &second))
                {
                        return (first > 0) ? first : second;
                }
                return std::nullopt;
        }

        constexpr void merge(const BoundingBox<N, T>& v)
        {
                bounds_[0] = ::ns::min(bounds_[0], v.bounds_[0]);
                bounds_[1] = ::ns::max(bounds_[1], v.bounds_[1]);
        }

        constexpr void merge(const Vector<N, T>& v)
        {
                bounds_[0] = ::ns::min(bounds_[0], v);
                bounds_[1] = ::ns::max(bounds_[1], v);
        }
};
}

namespace ns
{
template <std::size_t N, typename T>
std::string to_string(const geometry::BoundingBox<N, T>& box)
{
        return "(min " + to_string(box.min()) + ", max = " + to_string(box.max()) + ")";
}
}
