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

#include "compute_program.h"

#include "shader_source.h"

#include "com/groups.h"
#include "com/print.h"

#include <string>

constexpr int GROUP_SIZE = 16;

namespace gpu_opengl
{
namespace
{
std::string compute_source(int group_size)
{
        std::string s;
        s += "const uint GROUP_SIZE = " + to_string(group_size) + ";\n";
        return pencil_sketch_compute_comp(s);
}

std::string luminance_source(int group_size)
{
        std::string s;
        s += "const uint GROUP_SIZE = " + to_string(group_size) + ";\n";
        return pencil_sketch_luminance_comp(s);
}
}

PencilSketchProgramCompute::PencilSketchProgramCompute(const opengl::TextureRGBA32F& input, const opengl::TextureImage& objects,
                                                       const opengl::TextureRGBA32F& output)
        : m_groups_x(group_count(input.texture().width(), GROUP_SIZE)),
          m_groups_y(group_count(input.texture().height(), GROUP_SIZE)),
          m_program(opengl::ComputeShader(compute_source(GROUP_SIZE)))
{
        ASSERT(objects.format() == GL_R32UI);

        m_program.set_uniform_handle("img_input", input.image_resident_handle_read_only());
        m_program.set_uniform_handle("img_output", output.image_resident_handle_write_only());
        m_program.set_uniform_handle("img_objects", objects.image_resident_handle_read_only());
}

void PencilSketchProgramCompute::exec() const
{
        m_program.dispatch_compute(m_groups_x, m_groups_y, 1);
}

//

PencilSketchProgramLuminance::PencilSketchProgramLuminance(const opengl::TextureRGBA32F& output)
        : m_groups_x(group_count(output.texture().width(), GROUP_SIZE)),
          m_groups_y(group_count(output.texture().height(), GROUP_SIZE)),
          m_program(opengl::ComputeShader(luminance_source(GROUP_SIZE)))
{
        m_program.set_uniform_handle("img", output.image_resident_handle_read_write());
}

void PencilSketchProgramLuminance::exec() const
{
        m_program.dispatch_compute(m_groups_x, m_groups_y, 1);
}
}
