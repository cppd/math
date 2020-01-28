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

#if defined(OPENGL_FOUND)

#include "compute.h"

#include "compute_program.h"

namespace gpu_opengl
{
namespace
{
class Impl final : public PencilSketchCompute
{
        PencilSketchProgramCompute m_program_compute;

        void exec() override
        {
                m_program_compute.exec();
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }

public:
        Impl(const opengl::Texture& input,
             const opengl::Texture& objects,
             unsigned x,
             unsigned y,
             unsigned width,
             unsigned height,
             const opengl::Texture& output)
                : m_program_compute(input, objects, x, y, width, height, output)
        {
        }
};
}

std::unique_ptr<PencilSketchCompute> create_pencil_sketch_compute(
        const opengl::Texture& input,
        const opengl::Texture& objects,
        unsigned x,
        unsigned y,
        unsigned width,
        unsigned height,
        const opengl::Texture& output)
{
        return std::make_unique<Impl>(input, objects, x, y, width, height, output);
}
}

#endif
