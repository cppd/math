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

#include "geometry.h"

#include <src/com/error.h>

#include <algorithm>

namespace ns::gpu::renderer
{
namespace
{
constexpr double GRADIENT_H_IN_PIXELS = 0.5;

Vector3d world_volume_size(const Matrix4d& texture_to_world)
{
        // Example for x: texture_to_world * (1, 0, 0, 1) -> (x, y, z) -> length
        Vector3d size;
        for (unsigned i = 0; i < 3; ++i)
        {
                const Vector3d v(texture_to_world(0, i), texture_to_world(1, i), texture_to_world(2, i));
                size[i] = v.norm();
        }
        return size;
}
}

Vector4d volume_clip_plane(const Vector4d& world_clip_plane, const Matrix4d& model)
{
        Vector4d p = world_clip_plane * model;

        // from n * x + d with normal directed inward
        // to n * x - d with normal directed outward
        p[3] = -p[3];
        const Vector3d n = Vector3d(p[0], p[1], p[2]);
        return p / -n.norm();
}

// in texture coordinates
Vector3d volume_gradient_h(const Matrix4d& texture_to_world, const vulkan::Image& image)
{
        ASSERT(image.type() == VK_IMAGE_TYPE_3D);

        const Vector3d texture_pixel_size(
                1.0 / image.extent().width, 1.0 / image.extent().height, 1.0 / image.extent().depth);

        const Vector3d world_pixel_size(texture_pixel_size * world_volume_size(texture_to_world));

        double min_world_pixel_size = world_pixel_size[0];
        for (unsigned i = 1; i < 3; ++i)
        {
                min_world_pixel_size = std::min(min_world_pixel_size, world_pixel_size[i]);
        }

        min_world_pixel_size *= GRADIENT_H_IN_PIXELS;

        Vector3d h;
        for (unsigned i = 0; i < 3; ++i)
        {
                h[i] = (min_world_pixel_size / world_pixel_size[i]) * texture_pixel_size[i];
        }

        return h;
}
}
