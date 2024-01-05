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

4.3.2 The surface area heuristic
*/

#pragma once

#include "bvh_functions.h"
#include "bvh_object.h"

#include <src/com/error.h>
#include <src/com/type/limit.h>
#include <src/geometry/spatial/bounding_box.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <tuple>
#include <type_traits>

namespace ns::geometry::accelerators
{
namespace bvh_split_implementation
{
inline constexpr unsigned BUCKET_COUNT = 32;

template <std::size_t N, typename T>
class CenterBounds final
{
        spatial::BoundingBox<N, T> box_;
        unsigned axis_;
        T length_r_;
        T min_;

public:
        explicit CenterBounds(const std::span<const BvhObject<N, T>>& objects)
                : box_(compute_center_bounds(objects)),
                  axis_(box_.maximum_extent()),
                  length_r_(1 / (box_.max()[axis_] - box_.min()[axis_])),
                  min_(box_.min()[axis_])
        {
        }

        [[nodiscard]] bool is_point() const
        {
                return box_.min()[axis_] == box_.max()[axis_];
        }

        [[nodiscard]] unsigned axis() const
        {
                return axis_;
        }

        [[nodiscard]] unsigned bucket(const BvhObject<N, T>& object) const
        {
                const unsigned n = BUCKET_COUNT * ((object.center()[axis_] - min_) * length_r_);
                return std::min(n, BUCKET_COUNT - 1);
        }
};

template <std::size_t N, typename T>
struct Bucket final
{
        spatial::BoundingBox<N, T> bounds;
        T cost;

        Bucket()
        {
        }

        Bucket(const spatial::BoundingBox<N, T>& bounds, const T& cost)
                : bounds(bounds),
                  cost(cost)
        {
        }
};

template <std::size_t N, typename T>
std::tuple<std::array<std::optional<Bucket<N, T>>, BUCKET_COUNT>, T> compute_buckets_and_cost(
        const std::span<const BvhObject<N, T>> objects,
        const CenterBounds<N, T>& center_bounds)
{
        static_assert(BUCKET_COUNT >= 2);

        using SumType = std::common_type_t<double, T>;
        static_assert(Limits<SumType>::epsilon() < 1e-15L);

        std::array<SumType, BUCKET_COUNT> sum;
        SumType cost = 0;

        std::array<std::optional<Bucket<N, T>>, BUCKET_COUNT> buckets;
        for (const BvhObject<N, T>& object : objects)
        {
                cost += object.intersection_cost();
                const unsigned index = center_bounds.bucket(object);
                auto& bucket = buckets[index];
                if (bucket)
                {
                        bucket->bounds.merge(object.bounds());
                        sum[index] += object.intersection_cost();
                }
                else
                {
                        bucket.emplace().bounds = object.bounds();
                        sum[index] = object.intersection_cost();
                }
        }
        ASSERT(buckets.front());
        ASSERT(buckets.back());

        for (std::size_t i = 0; i < BUCKET_COUNT; ++i)
        {
                if (auto& bucket = buckets[i])
                {
                        bucket->cost = sum[i];
                }
        }

        return {buckets, cost};
}

template <std::size_t N, typename T>
std::array<Bucket<N, T>, BUCKET_COUNT - 1> incremental_bucket_sum_forward(
        const std::array<std::optional<Bucket<N, T>>, BUCKET_COUNT>& buckets)
{
        static_assert(BUCKET_COUNT >= 2);

        std::array<Bucket<N, T>, BUCKET_COUNT - 1> res;
        {
                const auto& bucket = buckets.front();
                ASSERT(bucket);
                res.front() = *bucket;
        }
        for (unsigned i = 1; i < BUCKET_COUNT - 1; ++i)
        {
                const auto& bucket = buckets[i];
                const Bucket<N, T>& previous = res[i - 1];
                Bucket<N, T>& current = res[i];
                if (!bucket)
                {
                        current = previous;
                        continue;
                }
                current = Bucket<N, T>(previous.bounds.merged(bucket->bounds), previous.cost + bucket->cost);
        }
        return res;
}

template <std::size_t N, typename T>
std::array<Bucket<N, T>, BUCKET_COUNT - 1> incremental_bucket_sum_backward(
        const std::array<std::optional<Bucket<N, T>>, BUCKET_COUNT>& buckets)
{
        static_assert(BUCKET_COUNT >= 2);

        std::array<Bucket<N, T>, BUCKET_COUNT - 1> res;
        {
                const auto& bucket = buckets.back();
                ASSERT(bucket);
                res.back() = *bucket;
        }
        for (unsigned i = BUCKET_COUNT - 2; i > 0; --i)
        {
                const auto& bucket = buckets[i];
                const Bucket<N, T>& previous = res[i];
                Bucket<N, T>& current = res[i - 1];
                if (!bucket)
                {
                        current = previous;
                        continue;
                }
                current = Bucket<N, T>(previous.bounds.merged(bucket->bounds), previous.cost + bucket->cost);
        }
        return res;
}

template <std::size_t N, typename T>
bool compare_cost(
        const T& cost,
        const std::array<Bucket<N, T>, BUCKET_COUNT - 1>& forward_sum,
        const std::array<Bucket<N, T>, BUCKET_COUNT - 1>& backward_sum)
{
        for (unsigned i = 0; i < forward_sum.size(); ++i)
        {
                const T relative_error = std::abs(1 - (forward_sum[i].cost + backward_sum[i].cost) / cost);
                if (!(relative_error < T{1e-5}))
                {
                        return false;
                }
        }
        return true;
}

template <std::size_t N, typename T>
std::tuple<T, unsigned> minimum_surface_area_heuristic_split(
        const spatial::BoundingBox<N, T>& bounds,
        const T& interior_node_traversal_cost,
        const std::array<Bucket<N, T>, BUCKET_COUNT - 1>& forward_sum,
        const std::array<Bucket<N, T>, BUCKET_COUNT - 1>& backward_sum)
{
        const T surface_r = 1 / bounds.surface();

        T split_cost = Limits<T>::max();
        unsigned index = Limits<unsigned>::max();
        for (unsigned i = 0; i < forward_sum.size(); ++i)
        {
                const T f = forward_sum[i].cost * forward_sum[i].bounds.surface();
                const T b = backward_sum[i].cost * backward_sum[i].bounds.surface();
                const T cost = interior_node_traversal_cost + (f + b) * surface_r;
                if (cost < split_cost)
                {
                        split_cost = cost;
                        index = i;
                }
        }
        ASSERT(index < forward_sum.size());
        return {split_cost, index};
}

template <std::size_t N, typename T>
auto partition(
        const std::span<BvhObject<N, T>> objects,
        const CenterBounds<N, T>& center_bounds,
        const unsigned split_index)
{
        const auto res = std::partition(
                objects.begin(), objects.end(),
                [&](const BvhObject<N, T>& object)
                {
                        return center_bounds.bucket(object) <= split_index;
                });
        ASSERT(res != objects.begin());
        ASSERT(res != objects.end());
        return res;
}
}

template <std::size_t N, typename T>
struct BvhSplit final
{
        std::span<BvhObject<N, T>> objects_min;
        std::span<BvhObject<N, T>> objects_max;
        spatial::BoundingBox<N, T> bounds_min;
        spatial::BoundingBox<N, T> bounds_max;
        unsigned axis;
};

template <std::size_t N, typename T>
std::optional<BvhSplit<N, T>> split(
        const std::span<BvhObject<N, T>> objects,
        const spatial::BoundingBox<N, T>& bounds,
        const T& interior_node_traversal_cost)
{
        namespace impl = bvh_split_implementation;

        if (objects.empty())
        {
                error("No BVH objects to split");
        }

        if (objects.size() == 1)
        {
                return {};
        }

        const impl::CenterBounds<N, T> center_bounds(objects);
        if (center_bounds.is_point())
        {
                return {};
        }

        const auto [buckets, cost] = impl::compute_buckets_and_cost<N, T>(objects, center_bounds);
        const auto forward_sum = impl::incremental_bucket_sum_forward(buckets);
        const auto backward_sum = impl::incremental_bucket_sum_backward(buckets);

        ASSERT(impl::compare_cost(cost, forward_sum, backward_sum));

        const auto [split_cost, split_index] = impl::minimum_surface_area_heuristic_split(
                bounds, interior_node_traversal_cost, forward_sum, backward_sum);
        if (split_cost >= cost)
        {
                return {};
        }

        const auto p = impl::partition(objects, center_bounds, split_index);

        return {
                {.objects_min = std::span(objects.data(), p - objects.begin()),
                 .objects_max = std::span(std::to_address(p), objects.end() - p),
                 .bounds_min = forward_sum[split_index].bounds,
                 .bounds_max = backward_sum[split_index].bounds,
                 .axis = center_bounds.axis()}
        };
}
}
