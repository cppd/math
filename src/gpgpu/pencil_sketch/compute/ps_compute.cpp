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

class ProgramCompute final
{
        static constexpr int GROUP_SIZE = 16;

        int m_groups_x;
        int m_groups_y;
        opengl::ComputeProgram m_program;

public:
        ProgramCompute(const opengl::TextureRGBA32F& input, bool input_is_srgb, const opengl::TextureImage& objects,
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

        void exec() const
        {
                m_program.dispatch_compute(m_groups_x, m_groups_y, 1);
        }
};

class ProgramLuminance final
{
        static constexpr int GROUP_SIZE = 16;

        int m_groups_x;
        int m_groups_y;
        opengl::ComputeProgram m_program;

public:
        ProgramLuminance(const opengl::TextureRGBA32F& output)
                : m_groups_x(group_count(output.texture().width(), GROUP_SIZE)),
                  m_groups_y(group_count(output.texture().height(), GROUP_SIZE)),
                  m_program(opengl::ComputeShader(luminance_source(GROUP_SIZE)))
        {
                m_program.set_uniform_handle("img", output.image_resident_handle_read_write());
        }

        void exec() const
        {
                m_program.dispatch_compute(m_groups_x, m_groups_y, 1);
        }
};

class Impl final : public gpgpu_opengl::PencilSketchCompute
{
        ProgramCompute m_program_compute;
        ProgramLuminance m_program_luminance;

        void exec() override
        {
                m_program_compute.exec();
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

                // Теперь в текстуре находится цвет RGB
                m_program_luminance.exec();
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }

public:
        Impl(const opengl::TextureRGBA32F& input, bool input_is_srgb, const opengl::TextureImage& objects,
             const opengl::TextureRGBA32F& output)
                : m_program_compute(input, input_is_srgb, objects, output), m_program_luminance(output)
        {
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
