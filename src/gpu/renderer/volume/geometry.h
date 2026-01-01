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

#include <src/geometry/spatial/hyperplane.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/vulkan/objects.h>

namespace ns::gpu::renderer
{
geometry::spatial::Hyperplane<3, double> volume_clip_plane(
        const numerical::Vector4d& world_clip_plane_equation,
        const numerical::Matrix4d& model);

// in texture coordinates
numerical::Vector3d volume_gradient_h(const numerical::Matrix4d& texture_to_world, const vulkan::Image& image);
}
