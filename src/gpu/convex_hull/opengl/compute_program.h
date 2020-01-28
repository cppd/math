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
class ConvexHullProgramPrepare final
{
        static constexpr int LINES_BINDING = 0;

        opengl::ComputeProgram m_program;
        const opengl::Buffer* m_lines = nullptr;
        unsigned m_height;

public:
        ConvexHullProgramPrepare(
                const opengl::Texture& objects,
                unsigned x,
                unsigned y,
                unsigned width,
                unsigned height,
                const opengl::Buffer& lines);

        void exec() const;
};

//

class ConvexHullProgramMerge final
{
        static constexpr int LINES_BINDING = 0;

        opengl::ComputeProgram m_program;
        const opengl::Buffer* m_lines = nullptr;

public:
        ConvexHullProgramMerge(unsigned height, const opengl::Buffer& lines);

        void exec() const;
};

//

class ConvexHullProgramFilter final
{
        static constexpr int LINES_BINDING = 0;
        static constexpr int POINTS_BINDING = 1;
        static constexpr int POINT_COUNT_BINDING = 2;

        opengl::ComputeProgram m_program;
        const opengl::Buffer* m_lines = nullptr;
        const opengl::Buffer* m_points = nullptr;
        const opengl::Buffer* m_point_count = nullptr;

public:
        ConvexHullProgramFilter(
                unsigned height,
                const opengl::Buffer& lines,
                const opengl::Buffer& points,
                const opengl::Buffer& point_count);

        void exec() const;
};
}

#endif
