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

namespace impl = gpgpu_convex_hull_compute_opengl_implementation;

namespace
{
int group_size_prepare(int width, unsigned max_group_size_x, unsigned max_group_invocations, unsigned max_shared_memory_size)
{
        unsigned shared_size_per_thread = 2 * sizeof(int32_t); // GLSL ivec2

        int max_group_size_limit = std::min(max_group_size_x, max_group_invocations);
        int max_group_size_memory = max_shared_memory_size / shared_size_per_thread;

        // максимально возможная степень 2
        int max_group_size = 1 << log_2(std::min(max_group_size_limit, max_group_size_memory));

        // один поток обрабатывает 2 и более пикселей, при этом число потоков должно быть степенью 2.
        int pref_thread_count = (width > 1) ? (1 << log_2(width - 1)) : 1;

        return (pref_thread_count <= max_group_size) ? pref_thread_count : max_group_size;
}

int group_size_merge(int height, unsigned max_group_size_x, unsigned max_group_invocations, unsigned max_shared_memory_size)
{
        static_assert(sizeof(float) == 4);

        unsigned shared_size_per_item = sizeof(float); // GLSL float

        if (max_shared_memory_size < height * shared_size_per_item)
        {
                error("Shared memory problem: needs " + to_string(height * shared_size_per_item) + ", exists " +
                      to_string(max_shared_memory_size));
        }

        int max_group_size = std::min(max_group_size_x, max_group_invocations);

        // Один поток первоначально обрабатывает группы до 4 элементов.
        int pref_thread_count = group_count(height, 4);

        return (pref_thread_count <= max_group_size) ? pref_thread_count : max_group_size;
}

int iteration_count_merge(int size)
{
        // Расчёт начинается с 4 элементов, правый средний индекс (начало второй половины) равен 2.
        // На каждой итерации индекс увеличивается в 2 раза.
        // Этот индекс должен быть строго меньше заданного числа size.
        // Поэтому число итераций равно максимальной степени 2, в которой число 2 строго меньше заданного числа size.
        return (size > 2) ? log_2(size - 1) : 0;
}

int group_size_prepare_opengl(int width)
{
        return group_size_prepare(width, opengl::max_fixed_group_size_x(), opengl::max_fixed_group_invocations(),
                                  opengl::max_compute_shared_memory());
}

int group_size_merge_opengl(int height)
{
        return group_size_merge(height, opengl::max_fixed_group_size_x(), opengl::max_fixed_group_invocations(),
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
        Impl(const opengl::TextureR32I& objects, const opengl::StorageBuffer& points)
                : m_height(objects.texture().height()),
                  m_prepare_prog(opengl::ComputeShader(
                          impl::prepare_constants(m_height, group_size_prepare_opengl(objects.texture().width())) +
                          prepare_shader)),
                  m_merge_prog(opengl::ComputeShader(
                          impl::merge_constants(m_height, group_size_merge_opengl(m_height), iteration_count_merge(m_height)) +
                          merge_shader)),
                  m_filter_prog(opengl::ComputeShader(impl::filter_constants(m_height) + filter_shader)),
                  m_lines(2 * m_height * sizeof(GLint)),
                  m_point_count(sizeof(GLint))
        {
                ASSERT(static_cast<unsigned>(points.size()) == (2 * m_height + 1) * (2 * sizeof(GLint)));

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
std::unique_ptr<ConvexHullCompute> create_convex_hull_compute(const opengl::TextureR32I& object_image,
                                                              const opengl::StorageBuffer& points)
{
        return std::make_unique<Impl>(object_image, points);
}
}
