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

namespace ns::geometry
{
namespace bvh_functions_implementation
{
template <std::size_t N, typename T>
const BvhObject<N, T>& bvh_object(const BvhObject<N, T>& v)
{
        return v;
};
template <std::size_t N, typename T>
BvhObject<N, T>&& bvh_object(BvhObject<N, T>&& v)
{
        return std::move(v);
}
}

template <typename T>
auto compute_bounds(const T& objects)
{
        namespace impl = bvh_functions_implementation;
        ASSERT(!objects.empty());
        BoundingBox box = impl::bvh_object(objects.front()).bounds;
        for (auto i = std::next(objects.begin()); i != objects.end(); ++i)
        {
                box.merge(impl::bvh_object(*i).bounds);
        }
        return box;
}

template <typename T>
auto compute_center_bounds(const T& objects)
{
        namespace impl = bvh_functions_implementation;
        ASSERT(!objects.empty());
        BoundingBox box = BoundingBox(impl::bvh_object(objects.front()).center);
        for (auto i = std::next(objects.begin()); i != objects.end(); ++i)
        {
                box.merge(impl::bvh_object(*i).center);
        }
        return box;
}

template <typename T>
auto compute_cost(const T& objects)
{
        namespace impl = bvh_functions_implementation;
        ASSERT(!objects.empty());
        auto cost = impl::bvh_object(objects.front()).intersection_cost;
        for (auto i = std::next(objects.begin()); i != objects.end(); ++i)
        {
                cost += impl::bvh_object(*i).intersection_cost;
        }
        return cost;
}
}
