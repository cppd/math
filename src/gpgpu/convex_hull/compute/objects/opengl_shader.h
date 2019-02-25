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

#pragma once

#include "graphics/opengl/buffers.h"
#include "graphics/opengl/shader.h"

namespace gpgpu_convex_hull_compute_opengl_implementation
{
class ProgramPrepare final
{
        static constexpr int LINES_BINDING = 0;

        opengl::ComputeProgram m_program;
        const opengl::StorageBuffer* m_lines = nullptr;
        unsigned m_height;

public:
        ProgramPrepare(const opengl::TextureImage& objects, const opengl::StorageBuffer& lines);

        void exec() const;
};

//

class ProgramMerge final
{
        static constexpr int LINES_BINDING = 0;

        opengl::ComputeProgram m_program;
        const opengl::StorageBuffer* m_lines = nullptr;

public:
        ProgramMerge(unsigned height, const opengl::StorageBuffer& lines);

        void exec() const;
};

//

class ProgramFilter final
{
        static constexpr int LINES_BINDING = 0;
        static constexpr int POINTS_BINDING = 1;
        static constexpr int POINT_COUNT_BINDING = 2;

        opengl::ComputeProgram m_program;
        const opengl::StorageBuffer* m_lines = nullptr;
        const opengl::StorageBuffer* m_points = nullptr;
        const opengl::StorageBuffer* m_point_count = nullptr;

public:
        ProgramFilter(unsigned height, const opengl::StorageBuffer& lines, const opengl::StorageBuffer& points,
                      const opengl::StorageBuffer& point_count);

        void exec() const;
};
}
