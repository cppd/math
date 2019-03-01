/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "graphics/opengl/buffers.h"

#include <memory>

namespace gpgpu_opengl
{
struct OpticalFlowCompute
{
        virtual ~OpticalFlowCompute() = default;

        virtual void reset() = 0;
        virtual bool exec() = 0;

        virtual GLuint64 image_pyramid_dx_texture() const = 0;
        virtual GLuint64 image_pyramid_texture() const = 0;
};

std::unique_ptr<OpticalFlowCompute> create_optical_flow_compute(int width, int height, const opengl::TextureRGBA32F& source_image,
                                                                int top_x, int top_y, const opengl::StorageBuffer& top_points,
                                                                const opengl::StorageBuffer& top_flow);
}
