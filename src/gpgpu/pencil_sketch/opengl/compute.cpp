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

#include "compute.h"

#include "compute_program.h"

namespace gpgpu_opengl
{
namespace
{
class Impl final : public PencilSketchCompute
{
        PencilSketchProgramCompute m_program_compute;
        PencilSketchProgramLuminance m_program_luminance;

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

std::unique_ptr<PencilSketchCompute> create_pencil_sketch_compute(const opengl::TextureRGBA32F& input, bool input_is_srgb,
                                                                  const opengl::TextureImage& objects,
                                                                  const opengl::TextureRGBA32F& output)
{
        return std::make_unique<Impl>(input, input_is_srgb, objects, output);
}
}
