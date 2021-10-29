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

#include "bvh_object.h"

#include "testing/bounding_box_intersection.h"

#include <src/com/error.h>
#include <src/com/type/limit.h>

#include <array>
#include <optional>
#include <vector>

namespace ns::geometry
{
namespace bvh_split_implementation
{
template <std::size_t BUCKET_COUNT, std::size_t N, typename T>
class Bounds
{
        BoundingBox<N, T> box_;
        unsigned axis_;
        T length_r_;
        T min_;
        T surface_;

public:
        Bounds(const std::vector<BvhObject<N, T>>& objects) : box_(objects[0].center)
        {
                for (std::size_t i = 0; i < objects.size(); ++i)
                {
                        box_.merge(objects[i].center);
                }
                axis_ = box_.maximum_extent();
                if (box_.min()[axis_] == box_.max()[axis_])
                {
                        return;
                }
                length_r_ = 1 / (box_.max()[axis_] - box_.min()[axis_]);
                min_ = box_.min()[axis_];
                surface_ = box_.surface();
        }

        bool empty() const
        {
                return box_.min()[axis_] == box_.max()[axis_];
        }

        unsigned axis() const
        {
                return axis_;
        }

        T surface() const
        {
                return surface_;
        }

        unsigned bucket(const BvhObject<N, T>& object) const
        {
                const std::size_t n = BUCKET_COUNT * ((object.center[axis_] - min_) * length_r_);
                return std::min(n, BUCKET_COUNT - 1);
        }
};

template <std::size_t N, typename T>
struct Bucket final
{
        BoundingBox<N, T> box;
        T cost;
        Bucket(const BoundingBox<N, T>& box, const T& cost) : box(box), cost(cost)
        {
        }
};

template <std::size_t BUCKET_COUNT, std::size_t N, typename T>
std::array<std::optional<Bucket<N, T>>, BUCKET_COUNT> compute_buckets(
        const std::vector<BvhObject<N, T>>& objects,
        const Bounds<BUCKET_COUNT, N, T>& bounds)
{
        static_assert(BUCKET_COUNT >= 2);

        std::array<std::optional<Bucket<N, T>>, BUCKET_COUNT> buckets;
        for (const BvhObject<N, T>& object : objects)
        {
                const unsigned bucket = bounds.bucket(object);
                if (buckets[bucket])
                {
                        buckets[bucket]->box.merge(object.box);
                        buckets[bucket]->cost += object.intersection_cost;
                }
                else
                {
                        buckets[bucket].emplace(object.box, object.intersection_cost);
                }
        }
        return buckets;
}

template <std::size_t BUCKET_COUNT, std::size_t N, typename T>
std::array<std::optional<Bucket<N, T>>, BUCKET_COUNT - 1> bucket_sum_low(
        const std::array<std::optional<Bucket<N, T>>, BUCKET_COUNT>& buckets)
{
        static_assert(BUCKET_COUNT >= 2);

        std::array<std::optional<Bucket<N, T>>, BUCKET_COUNT - 1> res;
        res.front() = buckets.front();
        for (unsigned i = 1; i < BUCKET_COUNT - 1; ++i)
        {
                const std::optional<Bucket<N, T>>& bucket = buckets[i];
                const std::optional<Bucket<N, T>>& previous = res[i - 1];
                std::optional<Bucket<N, T>>& current = res[i];
                if (!bucket)
                {
                        current = previous;
                }
                else if (!previous)
                {
                        current = bucket;
                }
                else
                {
                        current.emplace(bucket->box.merged(previous->box), bucket->cost + previous->cost);
                }
        }
        return res;
}

template <std::size_t BUCKET_COUNT, std::size_t N, typename T>
std::array<std::optional<Bucket<N, T>>, BUCKET_COUNT - 1> bucket_sum_high(
        const std::array<std::optional<Bucket<N, T>>, BUCKET_COUNT>& buckets)
{
        static_assert(BUCKET_COUNT >= 2);

        std::array<std::optional<Bucket<N, T>>, BUCKET_COUNT - 1> res;
        res.back() = buckets.back();
        for (unsigned i = BUCKET_COUNT - 2; i > 0; --i)
        {
                const std::optional<Bucket<N, T>>& bucket = buckets[i];
                const std::optional<Bucket<N, T>>& previous = res[i];
                std::optional<Bucket<N, T>>& current = res[i - 1];
                if (!bucket)
                {
                        current = previous;
                }
                else if (!previous)
                {
                        current = bucket;
                }
                else
                {
                        current.emplace(bucket->box.merged(previous->box), bucket->cost + previous->cost);
                }
        }
        return res;
}
}

template <std::size_t N, typename T>
void split(
        const std::vector<BvhObject<N, T>>& objects,
        unsigned* const axis,
        std::vector<BvhObject<N, T>>* const objects_min,
        std::vector<BvhObject<N, T>>* const objects_max)
{
        namespace impl = bvh_split_implementation;

        constexpr unsigned BUCKET_COUNT = 16;

        if (objects.empty())
        {
                error("No BVH objects to split");
        }

        objects_min->clear();
        objects_max->clear();

        impl::Bounds<BUCKET_COUNT, N, T> bounds(objects);
        if (bounds.empty())
        {
                return;
        }

        const std::array<std::optional<impl::Bucket<N, T>>, BUCKET_COUNT> buckets =
                impl::compute_buckets(objects, bounds);

        const std::array<std::optional<impl::Bucket<N, T>>, BUCKET_COUNT - 1> low = impl::bucket_sum_low(buckets);
        const std::array<std::optional<impl::Bucket<N, T>>, BUCKET_COUNT - 1> high = impl::bucket_sum_high(buckets);

        static const T bounding_box_cost =
                2 * spatial::testing::bounding_box::compute_intersections_r_per_second<N, T>();

        T min_cost = Limits<T>::max();
        unsigned min_index = Limits<unsigned>::max();
        for (unsigned i = 0; i < BUCKET_COUNT - 1; ++i)
        {
                ASSERT(low[i] || high[i]);
                if (!low[i] || !high[i])
                {
                        continue;
                }
                const T ls = low[i]->cost * low[i]->box.surface();
                const T hs = low[i]->cost * low[i]->box.surface();
                const T cost = bounding_box_cost + (ls + hs) / bounds.surface();
                if (cost < min_cost)
                {
                        min_cost = cost;
                        min_index = i;
                }
        }
        ASSERT(min_index < Limits<unsigned>::max());

        *axis = bounds.axis();
        for (const BvhObject<N, T>& object : objects)
        {
                if (bounds.bucket(object) <= min_index)
                {
                        objects_min->push_back(object);
                }
                else
                {
                        objects_max->push_back(object);
                }
        }
}
}
