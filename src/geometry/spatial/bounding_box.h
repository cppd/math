/*
Copyright (C) 2017-2023 Topological Manifold

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
#include <src/numerical/vector.h>

#include <array>
#include <optional>
#include <string>

namespace ns::geometry::spatial
{
namespace bounding_box_implementation
{
template <std::size_t M, std::size_t N, typename T>
[[nodiscard]] constexpr T volume_impl(const Vector<N, T>& d)
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
                return d[M - 1] * volume_impl<M - 1>(d);
        }
}

template <std::size_t M, std::size_t N, typename T>
[[nodiscard]] constexpr T surface_impl(const Vector<N, T>& d)
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
                return volume_impl<M - 1>(d) + d[M - 1] * surface_impl<M - 1>(d);
        }
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr T volume(const Vector<N, T>& d)
{
        return volume_impl<N>(d);
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr T surface(const Vector<N, T>& d)
{
        return 2 * surface_impl<N>(d);
}
}

template <std::size_t N, typename T>
class BoundingBox final
{
        static_assert(N >= 1);
        static_assert(std::is_floating_point_v<T>);
        static_assert(Limits<T>::is_iec559());

        std::array<Vector<N, T>, 2> bounds_;

        enum class IntersectionType
        {
                NEAREST,
                VOLUME
        };

        template <IntersectionType INTERSECTION_TYPE>
        [[nodiscard]] std::optional<T> intersect_impl(const Ray<N, T>& ray, const T max_distance) const
        {
                T near = 0;
                T far = max_distance;

                for (std::size_t i = 0; i < N; ++i)
                {
                        const T dir = ray.dir()[i];
                        const T d = ray.org()[i];
                        if (dir == 0)
                        {
                                if (d < bounds_[0][i] || d > bounds_[1][i])
                                {
                                        return {};
                                }
                                continue;
                        }
                        const bool dir_negative = (dir < 0);
                        const T r = 1 / dir;
                        const T a1 = (bounds_[dir_negative][i] - d) * r;
                        const T a2 = (bounds_[!dir_negative][i] - d) * r;
                        near = a1 > near ? a1 : near;
                        far = a2 < far ? a2 : far;
                        if (far < near)
                        {
                                return {};
                        }
                }

                static_assert(
                        INTERSECTION_TYPE == IntersectionType::NEAREST
                        || INTERSECTION_TYPE == IntersectionType::VOLUME);

                switch (INTERSECTION_TYPE)
                {
                case IntersectionType::NEAREST:
                        return near > 0 ? near : far;
                case IntersectionType::VOLUME:
                        return near;
                }
        }

        // if direction is +0 or -0, then dir_reciprocal must be +infinity.
        // In this case, a1 and a2 have values
        //   (infinity, infinity) -> near=infinity, far=far -> return false
        //   (NaN, infinity) -> near=near, far=far -> continue
        //   (-infinity, infinity) -> near=near, far=far -> continue
        //   (-infinity, NaN) -> near=near, far=far -> continue
        //   (-infinity, -infinity) -> near=near, far=-infinity -> return false
        [[nodiscard]] bool intersect_impl(
                const Vector<N, T>& org,
                const Vector<N, T>& dir_reciprocal,
                const Vector<N, bool>& dir_negative,
                const T max_distance) const
        {
                T near = 0;
                T far = max_distance;
                for (std::size_t i = 0; i < N; ++i)
                {
                        const T d = org[i];
                        const T r = dir_reciprocal[i];
                        const T a1 = (bounds_[dir_negative[i]][i] - d) * r;
                        const T a2 = (bounds_[!dir_negative[i]][i] - d) * r;
                        near = a1 > near ? a1 : near;
                        far = a2 < far ? a2 : far;
                        if (far < near)
                        {
                                return false;
                        }
                }
                return true;
        }

public:
        static T intersection_cost();
        static T intersection_volume_cost();
        static T intersection_r_cost();

        constexpr BoundingBox()
        {
        }

        constexpr BoundingBox(const Vector<N, T>& p1, const Vector<N, T>& p2)
                : bounds_{::ns::min(p1, p2), ::ns::max(p1, p2)}
        {
        }

        explicit constexpr BoundingBox(const Vector<N, T>& p)
                : bounds_{p, p}
        {
        }

        template <std::size_t SIZE>
        explicit constexpr BoundingBox(const std::array<Vector<N, T>, SIZE>& points)
                : BoundingBox(points[0])
        {
                static_assert(SIZE > 0);
                for (std::size_t i = 1; i < SIZE; ++i)
                {
                        merge(points[i]);
                }
        }

        template <std::size_t SIZE>
        BoundingBox(const std::vector<Vector<N, T>>& points, const std::array<int, SIZE>& indices)
                : BoundingBox(points[indices[0]])
        {
                static_assert(SIZE > 0);
                for (std::size_t i = 1; i < SIZE; ++i)
                {
                        merge(points[indices[i]]);
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
                return T{0.5} * (bounds_[0] + bounds_[1]);
        }

        [[nodiscard]] constexpr unsigned maximum_extent() const
        {
                const Vector<N, T> d = diagonal();
                T max = d[0];
                unsigned res = 0;
                for (std::size_t i = 1; i < N; ++i)
                {
                        if (d[i] > max)
                        {
                                max = d[i];
                                res = i;
                        }
                }
                return res;
        }

        [[nodiscard]] constexpr T volume() const
        {
                return bounding_box_implementation::volume(diagonal());
        }

        [[nodiscard]] constexpr T surface() const
                requires (N >= 2)
        {
                return bounding_box_implementation::surface(diagonal());
        }

        [[nodiscard]] std::optional<T> intersect(const Ray<N, T>& ray, const T max_distance = Limits<T>::max()) const
        {
                return intersect_impl<IntersectionType::NEAREST>(ray, max_distance);
        }

        [[nodiscard]] std::optional<T> intersect_volume(const Ray<N, T>& ray, const T max_distance = Limits<T>::max())
                const
        {
                return intersect_impl<IntersectionType::VOLUME>(ray, max_distance);
        }

        [[nodiscard]] bool intersect(
                const Vector<N, T>& org,
                const Vector<N, T>& dir_reciprocal,
                const Vector<N, bool>& dir_negative,
                const T max_distance = Limits<T>::max()) const
        {
                return intersect_impl(org, dir_reciprocal, dir_negative, max_distance);
        }

        constexpr void merge(const BoundingBox<N, T>& v)
        {
                bounds_[0] = ::ns::min(bounds_[0], v.bounds_[0]);
                bounds_[1] = ::ns::max(bounds_[1], v.bounds_[1]);
        }

        [[nodiscard]] constexpr BoundingBox<N, T> merged(const BoundingBox<N, T>& v) const
        {
                BoundingBox<N, T> res;
                res.bounds_[0] = ::ns::min(bounds_[0], v.bounds_[0]);
                res.bounds_[1] = ::ns::max(bounds_[1], v.bounds_[1]);
                return res;
        }

        constexpr void merge(const Vector<N, T>& v)
        {
                bounds_[0] = ::ns::min(bounds_[0], v);
                bounds_[1] = ::ns::max(bounds_[1], v);
        }

        [[nodiscard]] constexpr BoundingBox<N, T> merged(const Vector<N, T>& v) const
        {
                BoundingBox<N, T> res;
                res.bounds_[0] = ::ns::min(bounds_[0], v);
                res.bounds_[1] = ::ns::max(bounds_[1], v);
                return res;
        }

        [[nodiscard]] friend std::string to_string(const BoundingBox<N, T>& box)
        {
                return "(min " + to_string(box.min()) + ", max = " + to_string(box.max()) + ")";
        }
};
}
