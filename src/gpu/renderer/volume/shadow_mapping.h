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

#include "buffers/shadow_matrices.h"

#include <src/numerical/matrix.h>
#include <src/vulkan/device.h>

#include <vector>

namespace ns::gpu::renderer
{
class VolumeShadowMapping final
{
        Matrix4d world_to_shadow_ = Matrix4d(1);
        VolumeShadowMatricesBuffer buffer_;

public:
        VolumeShadowMapping(const vulkan::Device& device, const std::vector<std::uint32_t>& graphics_family_indices);

        const vulkan::Buffer& buffer() const;

        void set(const Matrix4d& world_to_shadow, const Matrix4d& texture_to_world, const Matrix4d& device_to_world);

        void set(const Matrix4d& texture_to_world, const Matrix4d& device_to_world) const;
};
}
