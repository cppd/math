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

#include "compute_memory.h"

#include "com/vec.h"

#include <graphics/opengl/shader.h>

namespace gpu_opengl
{
class OpticalFlowGrayscaleProgram final
{
        opengl::ComputeProgram m_program;

public:
        OpticalFlowGrayscaleProgram(const vec2i& group_size, unsigned x, unsigned y, unsigned width, unsigned height);

        void exec(const vec2i& groups, const OpticalFlowGrayscaleMemory& memory) const;
};

class OpticalFlowDownsampleProgram final
{
        opengl::ComputeProgram m_program;

public:
        OpticalFlowDownsampleProgram(const vec2i& group_size);

        void exec(const vec2i& groups, const OpticalFlowDownsampleMemory& memory) const;
};

class OpticalFlowSobelProgram final
{
        opengl::ComputeProgram m_program;

public:
        OpticalFlowSobelProgram(const vec2i& group_size);

        void exec(const vec2i& groups, const OpticalFlowSobelMemory& memory) const;
};

class OpticalFlowFlowProgram final
{
        opengl::ComputeProgram m_program;

public:
        OpticalFlowFlowProgram(const vec2i& group_size, int radius, int iteration_count, double stop_move_square,
                               double min_determinant);

        void exec(const vec2i& groups, const OpticalFlowDataMemory& data, const OpticalFlowImagesMemory& images) const;
};
}
