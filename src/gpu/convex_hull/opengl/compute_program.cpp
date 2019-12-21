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

#if defined(OPENGL_FOUND)

#include "compute_program.h"

#include "shader_source.h"

#include "com/print.h"
#include "gpu/convex_hull/com/com.h"
#include "graphics/opengl/functions.h"
#include "graphics/opengl/query.h"

#include <string>

namespace gpu_opengl
{
namespace
{
int group_size_prepare(int width)
{
        return convex_hull_group_size_prepare(width, opengl::max_fixed_group_size_x(), opengl::max_fixed_group_invocations(),
                                              opengl::max_compute_shared_memory());
}

int group_size_merge(int height)
{
        return convex_hull_group_size_merge(height, opengl::max_fixed_group_size_x(), opengl::max_fixed_group_invocations(),
                                            opengl::max_compute_shared_memory());
}

std::string prepare_source(unsigned x, unsigned y, unsigned width, unsigned height)
{
        int buffer_and_group_size = group_size_prepare(width);

        std::string s;
        s += "const int GROUP_SIZE = " + to_string(buffer_and_group_size) + ";\n";
        s += "const int BUFFER_SIZE = " + to_string(buffer_and_group_size) + ";\n";
        s += "const int X = " + to_string(x) + ";\n";
        s += "const int Y = " + to_string(y) + ";\n";
        s += "const int WIDTH = " + to_string(width) + ";\n";
        s += "const int HEIGHT = " + to_string(height) + ";\n";

        return convex_hull_prepare_comp(s);
}

std::string merge_source(unsigned height)
{
        int line_size = height;
        int group_size = group_size_merge(height);
        int iteration_count = convex_hull_iteration_count_merge(height);

        std::string s;
        s += "const int GROUP_SIZE = " + to_string(group_size) + ";\n";
        s += "const int LINE_SIZE = " + to_string(line_size) + ";\n";
        s += "const int ITERATION_COUNT = " + to_string(iteration_count) + ";\n";

        return convex_hull_merge_comp(s);
}

std::string filter_source(int height)
{
        int line_size = height;

        std::string s;
        s += "const int LINE_SIZE = " + to_string(line_size) + ";\n";

        return convex_hull_filter_comp(s);
}
}

ConvexHullProgramPrepare::ConvexHullProgramPrepare(const opengl::Texture& objects, unsigned x, unsigned y, unsigned width,
                                                   unsigned height, const opengl::Buffer& lines)
        : m_program(opengl::ComputeShader(prepare_source(x, y, width, height))), m_lines(&lines), m_height(height)
{
        ASSERT(objects.format() == GL_R32UI);

        ASSERT(width > 0 && height > 0);
        ASSERT(x + width <= static_cast<unsigned>(objects.width()));
        ASSERT(y + height <= static_cast<unsigned>(objects.height()));

        m_program.set_uniform_handle("objects", objects.image_handle_read_only());
}

void ConvexHullProgramPrepare::exec() const
{
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, LINES_BINDING, *m_lines);
        m_program.dispatch_compute(m_height, 1, 1);
}

//

ConvexHullProgramMerge::ConvexHullProgramMerge(unsigned height, const opengl::Buffer& lines)
        : m_program(opengl::ComputeShader(merge_source(height))), m_lines(&lines)
{
}

void ConvexHullProgramMerge::exec() const
{
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, LINES_BINDING, *m_lines);
        m_program.dispatch_compute(2, 1, 1);
}

//

ConvexHullProgramFilter::ConvexHullProgramFilter(unsigned height, const opengl::Buffer& lines, const opengl::Buffer& points,
                                                 const opengl::Buffer& point_count)
        : m_program(opengl::ComputeShader(filter_source(height))), m_lines(&lines), m_points(&points), m_point_count(&point_count)
{
}

void ConvexHullProgramFilter::exec() const
{
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, LINES_BINDING, *m_lines);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, POINTS_BINDING, *m_points);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, POINT_COUNT_BINDING, *m_point_count);
        m_program.dispatch_compute(1, 1, 1);
}
}

#endif
