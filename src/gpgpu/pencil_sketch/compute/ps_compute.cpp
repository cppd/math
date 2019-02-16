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

#include "ps_compute.h"

#include "com/print.h"
#include "gpgpu/com/groups.h"
#include "graphics/opengl/shader.h"

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

constexpr int GROUP_SIZE = 16;

namespace
{
std::string group_size_string()
{
        return "const uint GROUP_SIZE = " + to_string(GROUP_SIZE) + ";\n";
}

std::string compute_source(bool input_is_srgb)
{
        std::string s;
        s += group_size_string();
        s += std::string("const bool SOURCE_SRGB = ") + (input_is_srgb ? "true" : "false") + ";\n";
        return s + compute_shader;
}

std::string luminance_source()
{
        std::string s;
        s += group_size_string();
        return s + luminance_shader;
}

class Impl final : public gpgpu_opengl::PencilSketchCompute
{
        const int m_groups_x;
        const int m_groups_y;

        opengl::ComputeProgram m_compute_prog;
        opengl::ComputeProgram m_luminance_prog;

        void exec() override
        {
                m_compute_prog.dispatch_compute(m_groups_x, m_groups_y, 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

                // Теперь в текстуре находится цвет RGB
                m_luminance_prog.dispatch_compute(m_groups_x, m_groups_y, 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }

public:
        Impl(const opengl::TextureRGBA32F& input, bool input_is_srgb, const opengl::TextureImage& objects,
             const opengl::TextureRGBA32F& output)
                : m_groups_x(group_count(input.texture().width(), GROUP_SIZE)),
                  m_groups_y(group_count(input.texture().height(), GROUP_SIZE)),
                  m_compute_prog(opengl::ComputeShader(compute_source(input_is_srgb))),
                  m_luminance_prog(opengl::ComputeShader(luminance_source()))
        {
                ASSERT(objects.format() == GL_R32UI);

                m_compute_prog.set_uniform_handle("img_input", input.image_resident_handle_read_only());
                m_compute_prog.set_uniform_handle("img_output", output.image_resident_handle_write_only());
                m_compute_prog.set_uniform_handle("img_objects", objects.image_resident_handle_read_only());

                m_luminance_prog.set_uniform_handle("img", output.image_resident_handle_read_write());
        }
};
}

namespace gpgpu_opengl
{
std::unique_ptr<PencilSketchCompute> create_pencil_sketch_compute(const opengl::TextureRGBA32F& input, bool input_is_srgb,
                                                                  const opengl::TextureImage& objects,
                                                                  const opengl::TextureRGBA32F& output)
{
        return std::make_unique<Impl>(input, input_is_srgb, objects, output);
}
}
