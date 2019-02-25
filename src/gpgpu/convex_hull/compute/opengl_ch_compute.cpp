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

#include "opengl_ch_compute.h"

#include "com/bits.h"
#include "com/error.h"
#include "com/print.h"
#include "gpgpu/com/groups.h"
#include "gpgpu/convex_hull/compute/objects/com.h"
#include "gpgpu/convex_hull/compute/objects/opengl_shader.h"
#include "graphics/opengl/query.h"
#include "graphics/opengl/shader.h"

// clang-format off
constexpr const char filter_shader[]
{
#include "ch_filter.comp.str"
};
// clang-format on

namespace
{
namespace impl
{
using namespace gpgpu_convex_hull_compute_implementation;
using namespace gpgpu_convex_hull_compute_opengl_implementation;
}

class Impl final : public gpgpu_opengl::ConvexHullCompute
{
        opengl::ComputeProgram m_filter_prog;

        opengl::StorageBuffer m_lines;
        opengl::StorageBuffer m_point_count;

        impl::FilterMemory m_filter_memory;

        impl::ProgramPrepare m_program_prepare;
        impl::ProgramMerge m_program_merge;

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
                m_filter_memory.bind();
                m_filter_prog.dispatch_compute(1, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                GLint point_count;
                m_point_count.read(&point_count);

                return point_count;
        }

public:
        Impl(const opengl::TextureImage& objects, const opengl::StorageBuffer& points)
                : m_filter_prog(opengl::ComputeShader(impl::filter_constants(objects.height()) + filter_shader)),
                  m_lines(2 * objects.height() * sizeof(GLint)),
                  m_point_count(sizeof(GLint)),
                  m_program_prepare(objects, m_lines),
                  m_program_merge(objects.height(), m_lines)
        {
                ASSERT(static_cast<unsigned>(points.size()) == (2 * objects.height() + 1) * (2 * sizeof(GLint)));

                m_filter_memory.set_lines(m_lines);
                m_filter_memory.set_points(points);
                m_filter_memory.set_point_count(m_point_count);
        }

        Impl(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;
};
}

namespace gpgpu_opengl
{
std::unique_ptr<ConvexHullCompute> create_convex_hull_compute(const opengl::TextureImage& object_image,
                                                              const opengl::StorageBuffer& points)
{
        return std::make_unique<Impl>(object_image, points);
}
}
