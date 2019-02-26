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

#include "opengl_program.h"

#include "com/print.h"
#include "gpgpu/com/groups.h"

#include <string>

static constexpr int GROUP_SIZE = 16;

// clang-format off
constexpr const char compute_shader[]
{
#include "ps_compute.comp.str"
};
constexpr const char luminance_shader[]
{
#include "ps_luminance.comp.str"
};
// clang-format on

namespace
{
std::string compute_source(bool input_is_srgb, int group_size)
{
        std::string s;
        s += "const uint GROUP_SIZE = " + to_string(group_size) + ";\n";
        s += "const bool SOURCE_SRGB = " + std::string(input_is_srgb ? "true" : "false") + ";\n";
        return s + compute_shader;
}

std::string luminance_source(int group_size)
{
        std::string s;
        s += "const uint GROUP_SIZE = " + to_string(group_size) + ";\n";
        return s + luminance_shader;
}
}

namespace gpgpu_pencil_sketch_compute_opengl_implementation
{
ProgramCompute::ProgramCompute(const opengl::TextureRGBA32F& input, bool input_is_srgb, const opengl::TextureImage& objects,
                               const opengl::TextureRGBA32F& output)
        : m_groups_x(group_count(input.texture().width(), GROUP_SIZE)),
          m_groups_y(group_count(input.texture().height(), GROUP_SIZE)),
          m_program(opengl::ComputeShader(compute_source(input_is_srgb, GROUP_SIZE)))
{
        ASSERT(objects.format() == GL_R32UI);

        m_program.set_uniform_handle("img_input", input.image_resident_handle_read_only());
        m_program.set_uniform_handle("img_output", output.image_resident_handle_write_only());
        m_program.set_uniform_handle("img_objects", objects.image_resident_handle_read_only());
}

void ProgramCompute::exec() const
{
        m_program.dispatch_compute(m_groups_x, m_groups_y, 1);
}

//

ProgramLuminance::ProgramLuminance(const opengl::TextureRGBA32F& output)
        : m_groups_x(group_count(output.texture().width(), GROUP_SIZE)),
          m_groups_y(group_count(output.texture().height(), GROUP_SIZE)),
          m_program(opengl::ComputeShader(luminance_source(GROUP_SIZE)))
{
        m_program.set_uniform_handle("img", output.image_resident_handle_read_write());
}

void ProgramLuminance::exec() const
{
        m_program.dispatch_compute(m_groups_x, m_groups_y, 1);
}
}
