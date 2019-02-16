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

namespace
{
namespace impl
{
using namespace gpgpu_convex_hull_compute_implementation;
using namespace gpgpu_convex_hull_compute_opengl_implementation;
}

int group_size_prepare(int width)
{
        return impl::group_size_prepare(width, opengl::max_fixed_group_size_x(), opengl::max_fixed_group_invocations(),
                                        opengl::max_compute_shared_memory());
}

int group_size_merge(int height)
{
        return impl::group_size_merge(height, opengl::max_fixed_group_size_x(), opengl::max_fixed_group_invocations(),
                                      opengl::max_compute_shared_memory());
}

class Impl final : public gpgpu_opengl::ConvexHullCompute
{
        const int m_height;

        opengl::ComputeProgram m_prepare_prog;
        opengl::ComputeProgram m_merge_prog;
        opengl::ComputeProgram m_filter_prog;

        opengl::StorageBuffer m_lines;
        opengl::StorageBuffer m_point_count;

        impl::FilterMemory m_filter_memory;
        impl::MergeMemory m_merge_memory;
        impl::PrepareMemory m_prepare_memory;

        int exec() override
        {
                // Поиск минимума и максимума для каждой строки.
                // Если нет, то -1.
                m_prepare_memory.bind();
                m_prepare_prog.dispatch_compute(m_height, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                // Объединение оболочек, начиная от 4 элементов.
                m_merge_memory.bind();
                m_merge_prog.dispatch_compute(2, 1, 1);
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
                : m_height(objects.height()),
                  m_prepare_prog(opengl::ComputeShader(impl::prepare_constants(m_height, group_size_prepare(objects.width())) +
                                                       prepare_shader)),
                  m_merge_prog(opengl::ComputeShader(
                          impl::merge_constants(m_height, group_size_merge(m_height), impl::iteration_count_merge(m_height)) +
                          merge_shader)),
                  m_filter_prog(opengl::ComputeShader(impl::filter_constants(m_height) + filter_shader)),
                  m_lines(2 * m_height * sizeof(GLint)),
                  m_point_count(sizeof(GLint))
        {
                ASSERT(static_cast<unsigned>(points.size()) == (2 * m_height + 1) * (2 * sizeof(GLint)));
                ASSERT(objects.format() == GL_R32UI);

                m_filter_memory.set_lines(m_lines);
                m_filter_memory.set_points(points);
                m_filter_memory.set_point_count(m_point_count);

                m_merge_memory.set_lines(m_lines);

                m_prepare_memory.set_lines(m_lines);
                m_prepare_prog.set_uniform_handle("objects", objects.image_resident_handle_read_only());
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
