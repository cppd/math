/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "tree.h"

#include <src/com/error.h>
#include <src/com/reference.h>
#include <src/geometry/spatial/bounding_box.h>
#include <src/geometry/spatial/shape_overlap.h>

#include <cstddef>
#include <type_traits>
#include <vector>

namespace ns::geometry::accelerators
{
template <typename Parallelotope, typename Object>
class SpatialSubdivisionTreeObjects final : public SpatialSubdivisionTree<Parallelotope>::Objects
{
        static constexpr int N = Parallelotope::SPACE_DIMENSION;
        using T = Parallelotope::DataType;

        [[nodiscard]] static spatial::BoundingBox<N, T> bounding_box(const std::vector<Object>& objects)
        {
                if (objects.empty())
                {
                        error("No objects for tree");
                }

                spatial::BoundingBox<N, T> res = objects[0].bounding_box();
                for (std::size_t i = 1; i < objects.size(); ++i)
                {
                        res.merge(objects[i].bounding_box());
                }
                return res;
        }

        spatial::BoundingBox<N, T> bounding_box_;
        std::vector<std::remove_cvref_t<decltype(std::declval<Object>().overlap_function())>> overlap_functions_;

        [[nodiscard]] int count() const override
        {
                return overlap_functions_.size();
        }

        [[nodiscard]] const spatial::BoundingBox<N, T>& bounding_box() const override
        {
                return bounding_box_;
        }

        [[nodiscard]] std::vector<int> intersection_indices(
                const Parallelotope& parallelotope,
                const std::vector<int>& indices) const override
        {
                spatial::ShapeOverlap p(&parallelotope);
                std::vector<int> res;
                res.reserve(indices.size());
                for (const int index : indices)
                {
                        if (overlap_functions_[index](p))
                        {
                                res.push_back(index);
                        }
                }
                return res;
        }

public:
        explicit SpatialSubdivisionTreeObjects(const std::vector<Object>& objects)
                : bounding_box_(bounding_box(objects))
        {
                overlap_functions_.reserve(objects.size());
                for (const Object& object : objects)
                {
                        overlap_functions_.push_back(to_ref(object).overlap_function());
                }
        }
};
}
