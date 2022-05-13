/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "options.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/image/slice.h>
#include <src/model/volume_object.h>
#include <src/model/volume_utility.h>
#include <src/progress/progress_list.h>

namespace ns::process
{
template <std::size_t DIMENSION, std::size_t N>
void compute_slice(
        ProgressRatioList* const /*progress_list*/,
        const model::volume::VolumeObject<N>& volume_object,
        const std::vector<std::optional<int>>& slice_coordinates)
{
        static_assert(DIMENSION < N);

        if (slice_coordinates.size() != N)
        {
                error("Slice coordinate data size " + to_string(slice_coordinates.size())
                      + " is not equal to volume dimension " + to_string(N));
        }

        std::string object_name;

        std::array<image::Slice, N - DIMENSION> slices;
        std::size_t slice = 0;
        for (std::size_t dimension = 0; dimension < slice_coordinates.size(); ++dimension)
        {
                if (!slice_coordinates[dimension])
                {
                        continue;
                }
                if (slice >= slices.size())
                {
                        error("Error slice parameters");
                }
                slices[slice].dimension = dimension;
                slices[slice].coordinate = *slice_coordinates[dimension];
                object_name += "(" + to_string(dimension) + "," + to_string(*slice_coordinates[dimension]) + ")";
                ++slice;
        }
        if (slice != slices.size())
        {
                error("Error slice parameters");
        }

        std::unique_ptr<model::volume::Volume<DIMENSION>> volume = std::make_unique<model::volume::Volume<DIMENSION>>();

        {
                model::volume::Reading reading(volume_object);
                volume->image = image::slice(reading.volume().image, slices);
        }

        volume->matrix = model::volume::matrix_for_image_size(volume->image.size);

        const std::shared_ptr<model::volume::VolumeObject<DIMENSION>> obj =
                std::make_shared<model::volume::VolumeObject<DIMENSION>>(
                        std::move(volume),
                        model::volume::model_matrix_for_size_and_position(
                                *volume, SCENE_SIZE, SCENE_CENTER<DIMENSION, double>),
                        "Slice " + object_name);

        obj->insert(volume_object.id());
}
}
