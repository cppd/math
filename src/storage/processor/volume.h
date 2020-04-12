/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "../storage.h"

#include <src/model/volume_utility.h>
#include <src/numerical/vec.h>

#include <memory>

namespace storage::processor
{
template <size_t N, typename MeshFloat>
void compute(
        Storage<N, MeshFloat>* storage,
        std::unique_ptr<const volume::Volume<N>>&& volume,
        double object_size,
        const vec3& object_position)
{
        Matrix<N + 1, N + 1, double> matrix;
        if constexpr (N == 3)
        {
                ASSERT(object_size != 0);
                matrix = volume::model_matrix_for_size_and_position(*volume, object_size, object_position);
        }
        else
        {
                matrix = Matrix<N + 1, N + 1, double>(1);
        }

        std::shared_ptr<const volume::VolumeObject<N>> model_object =
                std::make_shared<const volume::VolumeObject<N>>(std::move(volume), matrix, "Volume");

        storage->set_volume_object(std::move(model_object));
}
}
