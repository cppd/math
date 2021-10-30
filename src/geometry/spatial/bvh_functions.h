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

#include <src/com/error.h>

#include <vector>

namespace ns::geometry
{
template <std::size_t N, typename T>
BoundingBox<N, T> compute_bounds(const std::vector<BvhObject<N, T>>& objects)
{
        ASSERT(!objects.empty());
        BoundingBox<N, T> box = BoundingBox<N, T>(objects.front().bounds);
        for (auto i = std::next(objects.cbegin()); i != objects.cend(); ++i)
        {
                box.merge(i->bounds);
        }
        return box;
}

template <std::size_t N, typename T>
BoundingBox<N, T> compute_center_bounds(const std::vector<BvhObject<N, T>>& objects)
{
        ASSERT(!objects.empty());
        BoundingBox<N, T> box = BoundingBox<N, T>(objects.front().center);
        for (auto i = std::next(objects.cbegin()); i != objects.cend(); ++i)
        {
                box.merge(i->center);
        }
        return box;
}

template <std::size_t N, typename T>
T compute_cost(const std::vector<BvhObject<N, T>>& objects)
{
        ASSERT(!objects.empty());
        T cost = objects.front().intersection_cost;
        for (auto i = std::next(objects.cbegin()); i != objects.cend(); ++i)
        {
                cost += i->intersection_cost;
        }
        return cost;
}
}
