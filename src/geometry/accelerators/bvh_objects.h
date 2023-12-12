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

#pragma once

#include "bvh_object.h"

#include "../spatial/bounding_box.h"

#include <src/com/reference.h>

#include <cstddef>
#include <type_traits>
#include <vector>

namespace ns::geometry::accelerators
{
namespace bvh_objects_implementation
{
template <typename T>
struct Type;

template <std::size_t N, typename T>
struct Type<spatial::BoundingBox<N, T>> final
{
        using Value = BvhObject<N, T>;
};
}

template <typename Objects>
auto bvh_objects(const Objects& objects)
{
        using BoundingBox = std::remove_cvref_t<decltype(to_ref(objects.front()).bounding_box())>;
        using BvhObject = bvh_objects_implementation::Type<BoundingBox>::Value;

        std::vector<BvhObject> res;
        res.reserve(objects.size());
        std::size_t i = 0;
        for (const auto& object : objects)
        {
                res.emplace_back(to_ref(object).bounding_box(), to_ref(object).intersection_cost(), i++);
        }
        return res;
}
}
