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

#include "shadow_mapping.h"

namespace ns::gpu::renderer
{
VolumeShadowMapping::VolumeShadowMapping(
        const vulkan::Device& device,
        const std::vector<std::uint32_t>& graphics_family_indices)
        : buffer_(device, graphics_family_indices)
{
}

const vulkan::Buffer& VolumeShadowMapping::buffer() const
{
        return buffer_.buffer();
}

void VolumeShadowMapping::set_matrix(const Matrix4d& world_to_shadow, const Matrix4d& texture_to_world)
{
        world_to_shadow_ = world_to_shadow;
        set_matrix(texture_to_world);
}

void VolumeShadowMapping::set_matrix(const Matrix4d& texture_to_world) const
{
        buffer_.set_matrix(world_to_shadow_ * texture_to_world);
}
}
