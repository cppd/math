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
std::string compute_source(unsigned group_size, unsigned x, unsigned y, unsigned width, unsigned height)
{
        std::string s;
        s += "const int GROUP_SIZE = " + to_string(group_size) + ";\n";
        s += "const int X = " + to_string(x) + ";\n";
        s += "const int Y = " + to_string(y) + ";\n";
        s += "const int WIDTH = " + to_string(width) + ";\n";
        s += "const int HEIGHT = " + to_string(height) + ";\n";
        return pencil_sketch_compute_comp(s);
}
}

PencilSketchProgramCompute::PencilSketchProgramCompute(const opengl::Texture& input, const opengl::Texture& objects, unsigned x,
                                                       unsigned y, unsigned width, unsigned height, const opengl::Texture& output)
        : m_groups_x(group_count(input.width(), GROUP_SIZE)),
          m_groups_y(group_count(input.height(), GROUP_SIZE)),
          m_program(opengl::ComputeShader(compute_source(GROUP_SIZE, x, y, width, height)))
{
        ASSERT(objects.format() == GL_R32UI);
        ASSERT(output.format() == GL_R32F);

        ASSERT(input.width() == objects.width() && input.height() == objects.height());
        ASSERT(static_cast<unsigned>(output.width()) == width && static_cast<unsigned>(output.height()) == height);
        ASSERT(width > 0 && height > 0);
        ASSERT(x + width <= static_cast<unsigned>(objects.width()));
        ASSERT(y + height <= static_cast<unsigned>(objects.height()));

        m_program.set_uniform_handle("src", input.texture_handle());
        m_program.set_uniform_handle("img_output", output.image_handle_write_only());
        m_program.set_uniform_handle("img_objects", objects.image_handle_read_only());
}

void PencilSketchProgramCompute::exec() const
{
        m_program.dispatch_compute(m_groups_x, m_groups_y, 1);
}
}
