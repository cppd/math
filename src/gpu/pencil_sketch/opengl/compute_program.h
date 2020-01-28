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
#include "graphics/opengl/shader.h"

namespace gpu_opengl
{
class PencilSketchProgramCompute final
{
        int m_groups_x;
        int m_groups_y;
        opengl::ComputeProgram m_program;

public:
        PencilSketchProgramCompute(
                const opengl::Texture& input,
                const opengl::Texture& objects,
                unsigned x,
                unsigned y,
                unsigned width,
                unsigned height,
                const opengl::Texture& output);

        void exec() const;
};
}

#endif
