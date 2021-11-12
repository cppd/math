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

#include "tree.h"

#include "../spatial/bounding_box.h"
#include "../spatial/shape_overlap.h"

#include <src/com/error.h>
#include <src/com/reference.h>

#include <vector>

namespace ns::geometry
{
template <typename Parallelotope, typename Object>
class SpatialSubdivisionTreeObjects final : public SpatialSubdivisionTree<Parallelotope>::Objects
{
        static constexpr int N = Parallelotope::SPACE_DIMENSION;
        using T = typename Parallelotope::DataType;

        static BoundingBox<N, T> bounding_box(const std::vector<Object>& objects)
        {
                if (objects.empty())
                {
                        error("No objects for tree");
                }
                BoundingBox<N, T> box = objects[0].bounding_box();
                for (std::size_t i = 1; i < objects.size(); ++i)
                {
                        box.merge(objects[i].bounding_box());
                }
                return box;
        }

        BoundingBox<N, T> bounding_box_;
        std::vector<std::remove_cvref_t<decltype(std::declval<Object>().overlap_function())>> overlap_functions_;

        int count() const override
        {
                return overlap_functions_.size();
        }

        const BoundingBox<N, T>& bounding_box() const override
        {
                return bounding_box_;
        }

        std::vector<int> intersection_indices(const Parallelotope& parallelotope, const std::vector<int>& indices)
                const override
        {
                ShapeOverlap p(&parallelotope);
                std::vector<int> intersections;
                intersections.reserve(indices.size());
                for (const int index : indices)
                {
                        if (overlap_functions_[index](p))
                        {
                                intersections.push_back(index);
                        }
                }
                return intersections;
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
