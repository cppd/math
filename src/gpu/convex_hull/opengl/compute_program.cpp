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

#include "com/print.h"
#include "gpu/convex_hull/com/com.h"
#include "graphics/opengl/query.h"

#include <string>

// clang-format off
constexpr const char prepare_shader[]
{
#include "ch_prepare.comp.str"
};
constexpr const char merge_shader[]
{
#include "ch_merge.comp.str"
};
constexpr const char filter_shader[]
{
#include "ch_filter.comp.str"
};
// clang-format on

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

std::string prepare_source(int width, int height)
{
        int line_size = height;
        int buffer_and_group_size = group_size_prepare(width);

        std::string s;
        s += "const uint GROUP_SIZE = " + to_string(buffer_and_group_size) + ";\n";
        s += "const uint LINE_SIZE = " + to_string(line_size) + ";\n";
        s += "const uint BUFFER_SIZE = " + to_string(buffer_and_group_size) + ";\n";
        s += '\n';
        return s + prepare_shader;
}

std::string merge_source(unsigned height)
{
        int line_size = height;
        int group_size = group_size_merge(height);
        int iteration_count = convex_hull_iteration_count_merge(height);

        std::string s;
        s += "const uint GROUP_SIZE = " + to_string(group_size) + ";\n";
        s += "const int LINE_SIZE = " + to_string(line_size) + ";\n";
        s += "const int ITERATION_COUNT = " + to_string(iteration_count) + ";\n";
        s += '\n';
        return s + merge_shader;
}

std::string filter_source(int height)
{
        int line_size = height;

        std::string s;
        s += "const int LINE_SIZE = " + to_string(line_size) + ";\n";
        s += '\n';
        return s + filter_shader;
}
}

ConvexHullProgramPrepare::ConvexHullProgramPrepare(const opengl::TextureImage& objects, const opengl::StorageBuffer& lines)
        : m_program(opengl::ComputeShader(prepare_source(objects.width(), objects.height()))),
          m_lines(&lines),
          m_height(objects.height())
{
        ASSERT(objects.format() == GL_R32UI);

        m_program.set_uniform_handle("objects", objects.image_resident_handle_read_only());
}

void ConvexHullProgramPrepare::exec() const
{
        m_lines->bind(LINES_BINDING);
        m_program.dispatch_compute(m_height, 1, 1);
}

//

ConvexHullProgramMerge::ConvexHullProgramMerge(unsigned height, const opengl::StorageBuffer& lines)
        : m_program(opengl::ComputeShader(merge_source(height))), m_lines(&lines)
{
}

void ConvexHullProgramMerge::exec() const
{
        m_lines->bind(LINES_BINDING);
        m_program.dispatch_compute(2, 1, 1);
}

//

ConvexHullProgramFilter::ConvexHullProgramFilter(unsigned height, const opengl::StorageBuffer& lines,
                                                 const opengl::StorageBuffer& points, const opengl::StorageBuffer& point_count)
        : m_program(opengl::ComputeShader(filter_source(height))), m_lines(&lines), m_points(&points), m_point_count(&point_count)
{
}

void ConvexHullProgramFilter::exec() const
{
        m_lines->bind(LINES_BINDING);
        m_points->bind(POINTS_BINDING);
        m_point_count->bind(POINT_COUNT_BINDING);
        m_program.dispatch_compute(1, 1, 1);
}
}
