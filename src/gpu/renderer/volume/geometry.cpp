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

#include "geometry.h"

#include <src/com/error.h>
#include <src/geometry/spatial/clip_plane.h>
#include <src/geometry/spatial/hyperplane.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>

namespace ns::gpu::renderer
{
namespace
{
constexpr double GRADIENT_H_IN_PIXELS = 0.5;

numerical::Vector3d world_volume_size(const numerical::Matrix4d& texture_to_world)
{
        // Example for x: texture_to_world * (1, 0, 0, 1) -> (x, y, z) -> length
        numerical::Vector3d size;
        for (unsigned i = 0; i < 3; ++i)
        {
                const numerical::Vector3d v(texture_to_world[0, i], texture_to_world[1, i], texture_to_world[2, i]);
                size[i] = v.norm();
        }
        return size;
}
}

geometry::spatial::Hyperplane<3, double> volume_clip_plane(
        const numerical::Vector4d& world_clip_plane_equation,
        const numerical::Matrix4d& model)
{
        return geometry::spatial::clip_plane_equation_to_clip_plane(world_clip_plane_equation * model);
}

// in texture coordinates
numerical::Vector3d volume_gradient_h(const numerical::Matrix4d& texture_to_world, const vulkan::Image& image)
{
        ASSERT(image.type() == VK_IMAGE_TYPE_3D);

        const numerical::Vector3d texture_pixel_size(
                1.0 / image.extent().width, 1.0 / image.extent().height, 1.0 / image.extent().depth);

        const numerical::Vector3d world_pixel_size(texture_pixel_size * world_volume_size(texture_to_world));

        double min_world_pixel_size = world_pixel_size[0];
        for (unsigned i = 1; i < 3; ++i)
        {
                min_world_pixel_size = std::min(min_world_pixel_size, world_pixel_size[i]);
        }

        min_world_pixel_size *= GRADIENT_H_IN_PIXELS;

        numerical::Vector3d h;
        for (unsigned i = 0; i < 3; ++i)
        {
                h[i] = (min_world_pixel_size / world_pixel_size[i]) * texture_pixel_size[i];
        }

        return h;
}
}
