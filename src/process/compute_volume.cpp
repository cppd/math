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

#include "compute_volume.h"

#include "options.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/image/slice.h>
#include <src/model/volume.h>
#include <src/model/volume_object.h>
#include <src/model/volume_utility.h>
#include <src/progress/progress_list.h>
#include <src/settings/instantiation.h>

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace ns::process
{
namespace
{
template <std::size_t DIMENSION, std::size_t N>
struct Slices final
{
        std::string object_name;
        std::array<image::Slice, N - DIMENSION> slices;
};

template <std::size_t DIMENSION, std::size_t N>
Slices<DIMENSION, N> create_slices(const std::vector<std::optional<int>>& slice_coordinates)
{
        if (slice_coordinates.size() != N)
        {
                error("Slice coordinate data size " + to_string(slice_coordinates.size())
                      + " is not equal to volume dimension " + to_string(N));
        }

        Slices<DIMENSION, N> res;

        std::size_t slice = 0;
        for (std::size_t dimension = 0; dimension < slice_coordinates.size(); ++dimension)
        {
                const auto& coordinates = slice_coordinates[dimension];
                if (!coordinates)
                {
                        continue;
                }
                if (slice >= res.slices.size())
                {
                        error("Error slice parameters");
                }
                res.slices[slice].dimension = dimension;
                res.slices[slice].coordinate = *coordinates;
                res.object_name += '(' + to_string(dimension) + ',' + to_string(*coordinates) + ')';
                ++slice;
        }

        if (slice != res.slices.size())
        {
                error("Error slice parameters");
        }

        return res;
}
}

template <std::size_t DIMENSION, std::size_t N>
void compute_slice(
        progress::RatioList* const /*progress_list*/,
        const model::volume::VolumeObject<N>& volume_object,
        const std::vector<std::optional<int>>& slice_coordinates)
{
        static_assert(DIMENSION > 0 && DIMENSION < N);

        const Slices<DIMENSION, N> slices = create_slices<DIMENSION, N>(slice_coordinates);

        auto volume = std::make_unique<model::volume::Volume<DIMENSION>>();

        {
                const model::volume::Reading reading(volume_object);
                volume->image = image::slice(reading.volume().image, slices.slices);
        }

        volume->matrix = model::volume::matrix_for_image_size(volume->image.size);

        const auto obj = std::make_shared<model::volume::VolumeObject<DIMENSION>>(
                std::move(volume),
                model::volume::model_matrix_for_size_and_position(*volume, SCENE_SIZE, SCENE_CENTER<DIMENSION, double>),
                "Slice " + slices.object_name);

        obj->insert(volume_object.id());
}

#define TEMPLATE(N, M)                     \
        template void compute_slice<M, N>( \
                progress::RatioList*, const model::volume::VolumeObject<N>&, const std::vector<std::optional<int>>&);

TEMPLATE_INSTANTIATION_N_M(TEMPLATE)
}
