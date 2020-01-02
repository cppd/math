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

#if defined(OPENGL_FOUND)

#include "graphics/opengl/buffers.h"

#include <memory>

namespace gpu_opengl
{
struct OpticalFlowCompute
{
        virtual ~OpticalFlowCompute() = default;

        virtual void reset() = 0;
        virtual void exec() = 0;

        virtual GLuint64 image_pyramid_dx_texture() const = 0;
        virtual GLuint64 image_pyramid_texture() const = 0;
};

std::unique_ptr<OpticalFlowCompute> create_optical_flow_compute(const opengl::Texture& source, unsigned x, unsigned y,
                                                                unsigned width, unsigned height, unsigned top_point_count_x,
                                                                unsigned top_point_count_y, const opengl::Buffer& top_points,
                                                                const opengl::Buffer& top_flow);
}

#endif
