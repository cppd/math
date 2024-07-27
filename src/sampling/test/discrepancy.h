/*
Copyright (C) 2017-2024 Topological Manifold

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

7.2.1 Evaluating sample patterns: discrepancy
*/

#pragma once

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <random>
#include <type_traits>
#include <vector>

namespace ns::sampling::test
{
template <std::size_t N, typename T>
class PointSearch final
{
        static bool inside(const numerical::Vector<N, T>& p, const std::array<std::array<T, 2>, N>& box)
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (!(p[i] >= box[i][0] && p[i] < box[i][1]))
                        {
                                return false;
                        }
                }
                return true;
        }

        std::vector<numerical::Vector<N, T>> points_;

public:
        explicit PointSearch(std::vector<numerical::Vector<N, T>> points)
                : points_(std::move(points))
        {
        }

        [[nodiscard]] int count_points(const std::array<std::array<T, 2>, N>& box) const
        {
                int count = 0;
                for (const numerical::Vector<N, T>& p : points_)
                {
                        if (inside(p, box))
                        {
                                ++count;
                        }
                }
                return count;
        }
};

template <std::size_t N, typename T>
void check_point_range(
        const numerical::Vector<N, T>& p,
        const std::type_identity_t<T>& min,
        const std::type_identity_t<T>& max)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!(p[i] >= min && p[i] < max))
                {
                        error("Point " + to_string(p) + " is not in the range [" + to_string(min) + ", "
                              + to_string(max) + ")");
                }
        }
}

template <typename T, typename RandomEngine>
std::array<T, 2> make_box_coordinates(
        const std::type_identity_t<T>& min,
        const std::type_identity_t<T>& max,
        RandomEngine& engine)
{
        T v0;
        T v1;
        do
        {
                v0 = std::uniform_real_distribution<T>(min, max)(engine);
                v1 = std::uniform_real_distribution<T>(v0, max)(engine);
        } while (!(v1 > v0));
        return {v0, v1};
}

template <std::size_t N, typename T, typename RandomEngine>
std::array<std::array<T, 2>, N> make_random_box(
        const std::type_identity_t<T>& min,
        const std::type_identity_t<T>& max,
        RandomEngine& engine)
{
        std::array<std::array<T, 2>, N> box;

        if (std::bernoulli_distribution(0.9)(engine))
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        const auto [v0, v1] = make_box_coordinates<T>(min, max, engine);
                        box[i][0] = v0;
                        box[i][1] = v1;
                }
        }
        else
        {
                const auto [v0, v1] = make_box_coordinates<T>(min, max, engine);
                for (std::size_t i = 0; i < N; ++i)
                {
                        box[i][0] = v0;
                        box[i][1] = v1;
                }
        }

        return box;
}

template <std::size_t N, typename T>
T compute_box_volume(const std::array<std::array<T, 2>, N>& box)
{
        T volume = 1;
        for (std::size_t i = 0; i < N; ++i)
        {
                ASSERT(box[i][1] > box[i][0]);
                volume *= box[i][1] - box[i][0];
        }
        return volume;
}

template <std::size_t N, typename T, typename RandomEngine>
T compute_discrepancy(
        const std::type_identity_t<T>& min,
        const std::type_identity_t<T>& max,
        const std::vector<numerical::Vector<N, T>>& points,
        const int box_count,
        RandomEngine& engine)
{
        if (!(max > min))
        {
                error("Max " + to_string(max) + " must be greater than min " + to_string(min));
        }

        for (const numerical::Vector<N, T>& p : points)
        {
                check_point_range(p, min, max);
        }

        const T box_min = min;
        const T box_max = std::nextafter(max, Limits<T>::max());
        const T volume = std::pow(box_max - box_min, T{N});

        const PointSearch<N, T> point_search(points);

        T max_discrepancy = Limits<T>::lowest();
        for (int i = 0; i < box_count; ++i)
        {
                const std::array<std::array<T, 2>, N> box = make_random_box<N, T>(box_min, box_max, engine);
                const T box_volume = compute_box_volume(box);
                const int point_count = point_search.count_points(box);
                const T discrepancy = std::abs(box_volume / volume - static_cast<T>(point_count) / points.size());
                max_discrepancy = std::max(discrepancy, max_discrepancy);
        }

        return max_discrepancy;
}
}
