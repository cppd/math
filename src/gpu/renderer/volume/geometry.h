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

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>

namespace ns::gpu::renderer
{
Vector4d volume_clip_plane(const Vector4d& world_clip_plane, const Matrix4d& model);

// in texture coordinates
Vector3d volume_gradient_h(const Matrix4d& texture_to_world, const vulkan::Image& image);
}