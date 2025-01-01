/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "computing.h"

#include "compute_meshes.h"
#include "compute_volume.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/gui/dialogs/bound_cocone.h>
#include <src/gui/dialogs/image_slice.h>
#include <src/model/mesh_object.h>
#include <src/model/volume_object.h>
#include <src/progress/progress_list.h>
#include <src/storage/types.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace ns::process
{
namespace
{
std::vector<int> volume_image_size(const storage::VolumeObjectConst& object)
{
        std::vector<int> size;
        std::visit(
                [&]<std::size_t N>(const std::shared_ptr<const model::volume::VolumeObject<N>>& volume_object)
                {
                        const model::volume::Reading reading(*volume_object);
                        size = [](const auto& array)
                        {
                                return std::vector(array.cbegin(), array.cend());
                        }(reading.volume().image.size);
                },
                object);
        return size;
}
}

std::function<void(progress::RatioList*)> action_bound_cocone(const storage::MeshObjectConst& object)
{
        std::optional<gui::dialogs::BoundCoconeParameters> parameters =
                gui::dialogs::BoundCoconeParametersDialog::show();
        if (!parameters)
        {
                return nullptr;
        }

        return std::visit(
                [&]<std::size_t N>(const std::shared_ptr<const model::mesh::MeshObject<N>>& mesh_object)
                {
                        std::function<void(progress::RatioList*)> f = [=](progress::RatioList* const progress_list)
                        {
                                constexpr bool CONVEX_HULL = false;
                                constexpr bool COCONE = false;
                                constexpr bool BOUND_COCONE = true;
                                constexpr bool MST = false;
                                compute_meshes(
                                        progress_list, CONVEX_HULL, COCONE, BOUND_COCONE, MST, *mesh_object,
                                        parameters->rho, parameters->alpha);
                        };
                        return f;
                },
                object);
}

std::function<void(progress::RatioList*)> action_3d_slice(const storage::VolumeObjectConst& object)
{
        constexpr std::size_t DIMENSION = 3;

        const std::vector<int> size = volume_image_size(object);
        if (size.size() <= DIMENSION)
        {
                error("Volume dimension (" + to_string(size.size()) + ") is not suitable for 3D slice");
        }

        std::optional<gui::dialogs::ImageSliceParameters> parameters =
                gui::dialogs::ImageSliceDialog::show(size, DIMENSION);
        if (!parameters)
        {
                return nullptr;
        }

        return std::visit(
                [&]<std::size_t N>(const std::shared_ptr<const model::volume::VolumeObject<N>>& volume_object)
                {
                        std::function<void(progress::RatioList*)> f = [=](progress::RatioList* const progress_list)
                        {
                                if constexpr (N > DIMENSION)
                                {
                                        compute_slice<DIMENSION>(progress_list, *volume_object, parameters->slices);
                                }
                        };
                        return f;
                },
                object);
}
}
