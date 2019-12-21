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

/*
По книге

Satyan L. Devadoss, Joseph O’Rourke.
Discrete and computational geometry.
Princeton University Press, 2011.

Chapter 2: CONVEX HULLS, 2.6 Divide-and-Conquer.
*/

#if defined(OPENGL_FOUND)

#include "compute.h"

#include "compute_program.h"

#include "com/error.h"

namespace gpu_opengl
{
namespace
{
class Impl final : public ConvexHullCompute
{
        opengl::Buffer m_lines;
        opengl::Buffer m_point_count;

        ConvexHullProgramPrepare m_program_prepare;
        ConvexHullProgramMerge m_program_merge;
        ConvexHullProgramFilter m_program_filter;

        int exec() override
        {
                // Поиск минимума и максимума для каждой строки.
                // Если нет, то -1.
                m_program_prepare.exec();
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                // Объединение оболочек, начиная от 4 элементов.
                m_program_merge.exec();
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                // Выбрасывание элементов со значением -1.
                m_program_filter.exec();
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                GLint point_count;
                opengl::map_and_read_from_buffer(m_point_count, &point_count);

                return point_count;
        }

public:
        Impl(const opengl::Texture& objects, unsigned x, unsigned y, unsigned width, unsigned height,
             const opengl::Buffer& points)
                : m_lines(2 * height * sizeof(GLint), 0),
                  m_point_count(sizeof(GLint), GL_MAP_READ_BIT),
                  m_program_prepare(objects, x, y, width, height, m_lines),
                  m_program_merge(height, m_lines),
                  m_program_filter(height, m_lines, points, m_point_count)
        {
                ASSERT(static_cast<unsigned>(points.size()) == (2 * height + 1) * (2 * sizeof(GLint)));
        }

        Impl(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;
};
}

std::unique_ptr<ConvexHullCompute> create_convex_hull_compute(const opengl::Texture& objects, unsigned x, unsigned y,
                                                              unsigned width, unsigned height, const opengl::Buffer& points)
{
        return std::make_unique<Impl>(objects, x, y, width, height, points);
}
}

#endif
