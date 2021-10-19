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

#pragma once

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

        Vector<N, T> min_;
        Vector<N, T> max_;

        bool intersect(const Ray<N, T>& r, T* const first, T* const second) const
        {
                T near = 0;
                T far = Limits<T>::max();

                for (unsigned i = 0; i < N; ++i)
                {
                        T s = r.dir()[i];
                        if (s == 0)
                        {
                                // parallel to the planes
                                T d = r.org()[i];
                                if (d < min_[i] || d > max_[i])
                                {
                                        // outside the planes
                                        return false;
                                }
                                // inside the planes
                                continue;
                        }

                        T d = r.org()[i];
                        T alpha1 = (min_[i] - d) / s;
                        T alpha2 = (max_[i] - d) / s;

                        if (s > 0)
                        {
                                // front intersection for the first plane
                                // back intersection for the second plane
                                near = std::max(alpha1, near);
                                far = std::min(alpha2, far);
                        }
                        else
                        {
                                // front intersection for the second plane
                                // back intersection for the first plane
                                near = std::max(alpha2, near);
                                far = std::min(alpha1, far);
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

public:
        constexpr BoundingBox(const Vector<N, T>& p1, const Vector<N, T>& p2)
                : min_(::ns::min(p1, p2)), max_(::ns::max(p1, p2))
        {
        }

        explicit constexpr BoundingBox(const Vector<N, T>& p) : min_(p), max_(p)
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
                return min_;
        }

        [[nodiscard]] constexpr const Vector<N, T>& max() const
        {
                return max_;
        }

        [[nodiscard]] constexpr Vector<N, T> diagonal() const
        {
                return max_ - min_;
        }

        [[nodiscard]] constexpr Vector<N, T> center() const
        {
                return T(0.5) * (max_ + min_);
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

        constexpr void merge(const BoundingBox<N, T>& v)
        {
                min_ = ::ns::min(min_, v.min_);
                max_ = ::ns::max(max_, v.max_);
        }

        constexpr void merge(const Vector<N, T>& v)
        {
                min_ = ::ns::min(min_, v);
                max_ = ::ns::max(max_, v);
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
