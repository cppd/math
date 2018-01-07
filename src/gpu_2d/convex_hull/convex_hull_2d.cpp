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

#include "convex_hull_2d.h"

#include "com/bits.h"
#include "com/error.h"
#include "com/math.h"
#include "com/time.h"
#include "graphics/query.h"

// clang-format off
constexpr const char vertex_shader[]
{
#include "convex_hull_2d.vert.str"
};
constexpr const char fragment_shader[]
{
#include "convex_hull_2d.frag.str"
};
constexpr const char prepare_shader[]
{
#include "ch_2d_prepare.comp.str"
};
constexpr const char merge_shader[]
{
#include "ch_2d_merge.comp.str"
};
constexpr const char filter_shader[]
{
#include "ch_2d_filter.comp.str"
};
// clang-format on

// rad / ms
constexpr double ANGULAR_FREQUENCY = TWO_PI * 5;

namespace
{
int get_group_size_prepare(int width, int shared_size_per_thread)
{
        int max_group_size_limit = std::min(get_max_work_group_size_x(), get_max_work_group_invocations());
        int max_group_size_memory = get_max_compute_shared_memory() / shared_size_per_thread;

        // максимально возможная степень 2
        int max_group_size = 1 << get_log_2(std::min(max_group_size_limit, max_group_size_memory));

        // один поток обрабатывает 2 и более пикселей, при этом число потоков должно быть степенью 2.
        int pref_thread_count = (width > 1) ? (1 << get_log_2(width - 1)) : 1;

        return (pref_thread_count <= max_group_size) ? pref_thread_count : max_group_size;
}

int get_group_size_merge(int height, int shared_size_per_item)
{
        if (get_max_compute_shared_memory() < height * shared_size_per_item)
        {
                error("Shared memory problem: needs " + std::to_string(height * shared_size_per_item) + ", exists " +
                      std::to_string(get_max_compute_shared_memory()));
        }

        int max_group_size = std::min(get_max_work_group_size_x(), get_max_work_group_invocations());

        // Один поток первоначально обрабатывает группы до 4 элементов.
        int pref_thread_count = get_group_count(height, 4);

        return (pref_thread_count <= max_group_size) ? pref_thread_count : max_group_size;
}

int get_iteration_count_merge(int size)
{
        // Расчёт начинается с 4 элементов, правый средний индекс (начало второй половины) равен 2.
        // На каждой итерации индекс увеличивается в 2 раза.
        // Этот индекс должен быть строго меньше заданного числа size.
        // Поэтому число итераций равно максимальной степени 2, в которой число 2 строго меньше заданного числа size.
        return (size > 2) ? get_log_2(size - 1) : 0;
}

std::string get_prepare_source(int width, int height, int group_size)
{
        std::string s;
        s += "const int WIDTH = " + std::to_string(width) + ";\n";
        s += "const int HEIGHT = " + std::to_string(height) + ";\n";
        s += "const int GROUP_SIZE = " + std::to_string(group_size) + ";\n";
        s += '\n';
        s += prepare_shader;
        return s;
}
std::string get_merge_source(int size, int group_size)
{
        std::string s;
        s += "const int SIZE = " + std::to_string(size) + ";\n";
        s += "const int GROUP_SIZE = " + std::to_string(group_size) + ";\n";
        s += "const int ITERATION_COUNT = " + std::to_string(get_iteration_count_merge(size)) + ";\n";
        s += '\n';
        s += merge_shader;
        return s;
}
std::string get_filter_source(int size)
{
        std::string s;
        s += "const int SIZE = " + std::to_string(size) + ";\n";
        s += '\n';
        s += filter_shader;
        return s;
}
}

class ConvexHull2D::Impl final
{
        const int m_width, m_height;
        const int m_group_size_prepare;
        const int m_group_size_merge;
        ComputeProgram m_prepare_prog, m_merge_prog, m_filter_prog;
        GraphicsProgram m_draw_prog;
        TextureR32F m_line_min, m_line_max;

        ShaderStorageBuffer m_points;
        TextureR32I m_point_count;
        double m_start_time;

public:
        Impl(const TextureR32I& objects, const mat4& mtx)
                : m_width(objects.get_texture().get_width()),
                  m_height(objects.get_texture().get_height()),
                  m_group_size_prepare(get_group_size_prepare(m_width, 2 * sizeof(GLint))),
                  m_group_size_merge(get_group_size_merge(m_height, sizeof(GLfloat))),
                  m_prepare_prog(ComputeShader(get_prepare_source(m_width, m_height, m_group_size_prepare))),
                  m_merge_prog(ComputeShader(get_merge_source(m_height, m_group_size_merge))),
                  m_filter_prog(ComputeShader(get_filter_source(m_height))),
                  m_draw_prog(VertexShader(vertex_shader), FragmentShader(fragment_shader)),
                  m_line_min(m_height, 1),
                  m_line_max(m_height, 1),
                  m_point_count(1, 1),
                  m_start_time(get_time_seconds())
        {
                m_prepare_prog.set_uniform_handle("objects", objects.get_image_resident_handle_read_only());
                m_prepare_prog.set_uniform_handle("line_min", m_line_min.get_image_resident_handle_write_only());
                m_prepare_prog.set_uniform_handle("line_max", m_line_max.get_image_resident_handle_write_only());

                m_merge_prog.set_uniform_handles("lines", {m_line_min.get_image_resident_handle_read_write(),
                                                           m_line_max.get_image_resident_handle_read_write()});

                m_points.create_dynamic_copy((2 * m_height) * (2 * sizeof(GLfloat)));

                m_filter_prog.set_uniform_handle("line_min", m_line_min.get_image_resident_handle_read_only());
                m_filter_prog.set_uniform_handle("line_max", m_line_max.get_image_resident_handle_read_only());
                m_filter_prog.set_uniform_handle("points_count", m_point_count.get_image_resident_handle_write_only());

                m_draw_prog.set_uniform_float("mvpMatrix", mtx);
        }

        void reset_timer()
        {
                m_start_time = get_time_seconds();
        }

        void draw()
        {
                m_points.bind(0);

                // Поиск минимума и максимума для каждой строки.
                // Если нет, то -1.
                m_prepare_prog.dispatch_compute(m_height, 1, 1, m_group_size_prepare, 1, 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

                // Объединение оболочек, начиная от 4 элементов.
                m_merge_prog.dispatch_compute(2, 1, 1, m_group_size_merge, 1, 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

                // Выбрасывание элементов со значением -1.
                m_filter_prog.dispatch_compute(1, 1, 1, 1, 1, 1);

                glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
                GLint point_count;
                m_point_count.get_texture_sub_image(0, 0, 1, 1, &point_count);

                float d = 0.5 + 0.5 * std::sin(ANGULAR_FREQUENCY * (get_time_seconds() - m_start_time));
                m_draw_prog.set_uniform("brightness", d);

                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
                m_draw_prog.draw_arrays(GL_LINE_LOOP, 0, point_count);
        }
};

ConvexHull2D::ConvexHull2D(const TextureR32I& objects, const mat4& mtx) : m_impl(std::make_unique<Impl>(objects, mtx))
{
}
ConvexHull2D::~ConvexHull2D() = default;

void ConvexHull2D::reset_timer()
{
        m_impl->reset_timer();
}
void ConvexHull2D::draw()
{
        m_impl->draw();
}
