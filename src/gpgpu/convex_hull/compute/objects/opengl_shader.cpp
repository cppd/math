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

#include "opengl_shader.h"

#include "com.h"

#include "com/print.h"
#include "graphics/opengl/query.h"

// clang-format off
constexpr const char prepare_shader[]
{
#include "ch_prepare.comp.str"
};
// clang-format on

namespace
{
int group_size_prepare(int width)
{
        namespace impl = gpgpu_convex_hull_compute_implementation;
        return impl::group_size_prepare(width, opengl::max_fixed_group_size_x(), opengl::max_fixed_group_invocations(),
                                        opengl::max_compute_shared_memory());
}

std::string prepare_source(int line_size, int buffer_and_group_size)
{
        std::string s;
        s += "const uint GROUP_SIZE = " + to_string(buffer_and_group_size) + ";\n";
        s += "const uint LINE_SIZE = " + to_string(line_size) + ";\n";
        s += "const uint BUFFER_SIZE = " + to_string(buffer_and_group_size) + ";\n";
        s += '\n';
        return s + prepare_shader;
}
}

namespace gpgpu_convex_hull_compute_opengl_implementation
{
ProgramPrepare::ProgramPrepare(const opengl::TextureImage& objects, const opengl::StorageBuffer& lines)
        : m_program(opengl::ComputeShader(prepare_source(objects.height(), group_size_prepare(objects.width())))),
          m_height(objects.height())
{
        ASSERT(objects.format() == GL_R32UI);

        m_lines = &lines;
        m_program.set_uniform_handle("objects", objects.image_resident_handle_read_only());
}

void ProgramPrepare::exec() const
{
        m_lines->bind(LINES_BINDING);
        m_program.dispatch_compute(m_height, 1, 1);
}

//

void MergeMemory::set_lines(const opengl::StorageBuffer& lines)
{
        m_lines = &lines;
}

void MergeMemory::bind() const
{
        ASSERT(m_lines);

        m_lines->bind(LINES_BINDING);
}

std::string merge_constants(int line_size, int group_size, int iteration_count)
{
        std::string s;
        s += "const uint GROUP_SIZE = " + to_string(group_size) + ";\n";
        s += "const int LINE_SIZE = " + to_string(line_size) + ";\n";
        s += "const int ITERATION_COUNT = " + to_string(iteration_count) + ";\n";
        s += '\n';
        return s;
}

//

void FilterMemory::set_lines(const opengl::StorageBuffer& lines)
{
        m_lines = &lines;
}

void FilterMemory::set_points(const opengl::StorageBuffer& points)
{
        m_points = &points;
}

void FilterMemory::set_point_count(const opengl::StorageBuffer& point_count)
{
        m_point_count = &point_count;
}

void FilterMemory::bind() const
{
        ASSERT(m_lines && m_points && m_point_count);

        m_lines->bind(LINES_BINDING);
        m_points->bind(POINTS_BINDING);
        m_point_count->bind(POINT_COUNT_BINDING);
}

std::string filter_constants(int line_size)
{
        std::string s;
        s += "const int LINE_SIZE = " + to_string(line_size) + ";\n";
        s += '\n';
        return s;
}
}
