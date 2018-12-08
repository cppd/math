/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "ch_gl2d.h"

#include "com/bits.h"
#include "gpgpu/com/groups.h"
#include "graphics/opengl/query.h"

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
int group_size_prepare(int width, int shared_size_per_thread)
{
        int max_group_size_limit = std::min(opengl::max_fixed_group_size_x(), opengl::max_fixed_group_invocations());
        int max_group_size_memory = opengl::max_compute_shared_memory() / shared_size_per_thread;

        // максимально возможная степень 2
        int max_group_size = 1 << log_2(std::min(max_group_size_limit, max_group_size_memory));

        // один поток обрабатывает 2 и более пикселей, при этом число потоков должно быть степенью 2.
        int pref_thread_count = (width > 1) ? (1 << log_2(width - 1)) : 1;

        return (pref_thread_count <= max_group_size) ? pref_thread_count : max_group_size;
}

int group_size_merge(int height, int shared_size_per_item)
{
        if (opengl::max_compute_shared_memory() < height * shared_size_per_item)
        {
                error("Shared memory problem: needs " + std::to_string(height * shared_size_per_item) + ", exists " +
                      std::to_string(opengl::max_compute_shared_memory()));
        }

        int max_group_size = std::min(opengl::max_fixed_group_size_x(), opengl::max_fixed_group_invocations());

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

std::string group_size_string(int group_size)
{
        return "const uint GROUP_SIZE = " + std::to_string(group_size) + ";\n";
}

std::string prepare_source(int line_size, int group_size)
{
        std::string s;
        s += group_size_string(group_size);
        s += "const int LINE_SIZE = " + std::to_string(line_size) + ";\n";
        s += '\n';
        return s + prepare_shader;
}

std::string merge_source(int line_size, int group_size)
{
        std::string s;
        s += group_size_string(group_size);
        s += "const int LINE_SIZE = " + std::to_string(line_size) + ";\n";
        s += '\n';
        return s + merge_shader;
}

std::string filter_source(int line_size)
{
        std::string s;
        s += "const int LINE_SIZE = " + std::to_string(line_size) + ";\n";
        s += '\n';
        return s + filter_shader;
}

class Impl final : public ConvexHullGL2D
{
        const int m_height;
        const opengl::ShaderStorageBuffer& m_points;

        opengl::ComputeProgram m_prepare_prog;
        opengl::ComputeProgram m_merge_prog;
        opengl::ComputeProgram m_filter_prog;

        opengl::ShaderStorageBuffer m_lines;
        opengl::ShaderStorageBuffer m_point_count;

        int exec() override
        {
                m_lines.bind(0);
                m_points.bind(1);
                m_point_count.bind(2);

                // Поиск минимума и максимума для каждой строки.
                // Если нет, то -1.
                m_prepare_prog.dispatch_compute(m_height, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                // Объединение оболочек, начиная от 4 элементов.
                m_merge_prog.dispatch_compute(2, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                // Выбрасывание элементов со значением -1.
                m_filter_prog.dispatch_compute(1, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

                GLint point_count;
                m_point_count.read(&point_count);

                return point_count;
        }

public:
        Impl(const opengl::TextureR32I& objects, const opengl::ShaderStorageBuffer& points)
                : m_height(objects.texture().height()),
                  m_points(points),
                  m_prepare_prog(opengl::ComputeShader(
                          prepare_source(m_height, group_size_prepare(objects.texture().width(), 2 * sizeof(GLint))))),
                  m_merge_prog(opengl::ComputeShader(merge_source(m_height, group_size_merge(m_height, sizeof(GLfloat))))),
                  m_filter_prog(opengl::ComputeShader(filter_source(m_height)))
        {
                m_prepare_prog.set_uniform_handle("objects", objects.image_resident_handle_read_only());

                m_merge_prog.set_uniform("iteration_count", iteration_count_merge(m_height));

                m_lines.create_dynamic_copy(2 * m_height * sizeof(GLfloat));
                m_point_count.create_dynamic_copy(sizeof(GLint));
        }
};
}

std::unique_ptr<ConvexHullGL2D> create_convex_hull_gl2d(const opengl::TextureR32I& object_image,
                                                        const opengl::ShaderStorageBuffer& points)
{
        return std::make_unique<Impl>(object_image, points);
}
