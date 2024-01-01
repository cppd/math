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

#pragma once

#include "../spatial/bounding_box.h"

#include <src/com/error.h>

namespace ns::geometry::accelerators
{
template <typename T>
auto compute_bounds(const T& objects)
{
        ASSERT(!objects.empty());

        spatial::BoundingBox res{objects.front().bounds()};
        for (auto iter = std::next(objects.begin()); iter != objects.end(); ++iter)
        {
                res.merge(iter->bounds());
        }
        return res;
}

template <typename T>
auto compute_center_bounds(const T& objects)
{
        ASSERT(!objects.empty());

        spatial::BoundingBox res{objects.front().center()};
        for (auto iter = std::next(objects.begin()); iter != objects.end(); ++iter)
        {
                res.merge(iter->center());
        }
        return res;
}
}
