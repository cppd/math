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

#if defined(OPENGL_FOUND)

#include "compute_program.h"

#include "shader_source.h"

namespace gpu_opengl
{
namespace
{
std::string grayscale_source(const vec2i& group_size, unsigned x, unsigned y, unsigned width, unsigned height)
{
        ASSERT(group_size[0] == group_size[1]);

        std::string s;
        s += "const uint GROUP_SIZE = " + to_string(group_size[0]) + ";\n";
        s += "const int X = " + to_string(x) + ";\n";
        s += "const int Y = " + to_string(y) + ";\n";
        s += "const int WIDTH = " + to_string(width) + ";\n";
        s += "const int HEIGHT = " + to_string(height) + ";\n";
        return optical_flow_grayscale_comp(s);
}

std::string downsample_source(const vec2i& group_size)
{
        ASSERT(group_size[0] == group_size[1]);

        std::string s;
        s += "const uint GROUP_SIZE = " + to_string(group_size[0]) + ";\n";
        return optical_flow_downsample_comp(s);
}

std::string sobel_source(const vec2i& group_size)
{
        ASSERT(group_size[0] == group_size[1]);

        std::string s;
        s += "const uint GROUP_SIZE = " + to_string(group_size[0]) + ";\n";
        return optical_flow_sobel_comp(s);
}

std::string flow_source(const vec2i& group_size, int radius, int iteration_count, double stop_move_square, double min_determinant)
{
        ASSERT(group_size[0] == group_size[1]);

        std::string s;
        s += "const uint GROUP_SIZE = " + to_string(group_size[0]) + ";\n";
        s += "const int RADIUS = " + to_string(radius) + ";\n";
        s += "const int ITERATION_COUNT = " + to_string(iteration_count) + ";\n";
        s += "const float STOP_MOVE_SQUARE = " + to_string(stop_move_square) + ";\n";
        s += "const float MIN_DETERMINANT = " + to_string(min_determinant) + ";\n";
        return optical_flow_flow_comp(s);
}
}

//

OpticalFlowGrayscaleProgram::OpticalFlowGrayscaleProgram(const vec2i& group_size, unsigned x, unsigned y, unsigned width,
                                                         unsigned height)
        : m_program(opengl::ComputeShader(grayscale_source(group_size, x, y, width, height)))
{
}

void OpticalFlowGrayscaleProgram::exec(const vec2i& groups, const OpticalFlowGrayscaleMemory& memory) const
{
        memory.bind();
        m_program.dispatch_compute(groups[0], groups[1], 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

//

OpticalFlowDownsampleProgram::OpticalFlowDownsampleProgram(const vec2i& group_size)
        : m_program(opengl::ComputeShader(downsample_source(group_size)))
{
}

void OpticalFlowDownsampleProgram::exec(const vec2i& groups, const OpticalFlowDownsampleMemory& memory) const
{
        memory.bind();
        m_program.dispatch_compute(groups[0], groups[1], 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

//

OpticalFlowSobelProgram::OpticalFlowSobelProgram(const vec2i& group_size)
        : m_program(opengl::ComputeShader(sobel_source(group_size)))
{
}

void OpticalFlowSobelProgram::exec(const vec2i& groups, const OpticalFlowSobelMemory& memory) const
{
        memory.bind();
        m_program.dispatch_compute(groups[0], groups[1], 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

//

OpticalFlowFlowProgram::OpticalFlowFlowProgram(const vec2i& group_size, int radius, int iteration_count, double stop_move_square,
                                               double min_determinant)
        : m_program(opengl::ComputeShader(flow_source(group_size, radius, iteration_count, stop_move_square, min_determinant)))
{
}

void OpticalFlowFlowProgram::exec(const vec2i& groups, const OpticalFlowDataMemory& data,
                                  const OpticalFlowImagesMemory& images) const
{
        data.bind();
        images.bind();
        m_program.dispatch_compute(groups[0], groups[1], 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}
}

#endif
