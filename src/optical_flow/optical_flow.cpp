/*
Copyright (C) 2017 Topological Manifold

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

Aaftab Munshi, Benedict R. Gaster, Timothy G. Mattson, James Fung, Dan Ginsburg.
OpenCL Programming Guide.
Addison-Wesley, 2011.
Chapter 19. Optical Flow.

Дополнительная информация

Salil Kapur, Nisarg Thakkar.
Mastering OpenCV Android Application Programming.
Packt Publishing, 2015.
Chapter 5. Tracking Objects in Videos.
*/

#include "optical_flow.h"

#include "com/error.h"
#include "com/log.h"
#include "com/math.h"
#include "com/print.h"
#include "com/time.h"

#include <array>
#include <glm/vec2.hpp>
#include <limits>
#include <vector>

// clang-format off
constexpr const char vertex_shader[]
{
#include "optical_flow.vert.str"
};
constexpr const char fragment_shader[]
{
#include "optical_flow.frag.str"
};
constexpr const char vertex_debug_shader[]
{
#include "optical_flow_debug.vert.str"
};
constexpr const char fragment_debug_shader[]
{
#include "optical_flow_debug.frag.str"
};
constexpr const char sobel_compute_shader[]
{
#include "of_sobel.comp.str"
};
constexpr const char flow_compute_shader[]
{
#include "of_flow.comp.str"
};
constexpr const char downsample_compute_shader[]
{
#include "of_downsample.comp.str"
};
constexpr const char grayscale_compute_shader[]
{
#include "of_grayscale.comp.str"
};
constexpr const char lines_compute_shader[]
{
#include "of_lines.comp.str"
};
// clang-format on

// Размер по X и по Y группы потоков вычислительных шейдеров
constexpr int GROUP_SIZE = 16;

// Минимальный размер изображения для пирамиды изображений
constexpr int BOTTOM_IMAGE_SIZE = 16;
// Расстояние между точками потока на экране
constexpr int POINT_DISTANCE = 8;

// Интервал ожидания для расчёта потока не для каждого кадра
// constexpr double COMPUTE_INTERVAL_SECONDS = 1.0 / 10;

// Параметры алгоритма для передачи в вычислительный шейдер
// Радиус окрестности точки
constexpr int RADIUS = 6;
// Максимальное количество итераций
constexpr int ITERATION_COUNT = 10;
// Если на итерации квадрат потока меньше этого значения, то выход из цикла
constexpr float STOP_MOVE_SQUARE = square(1e-3);
// Если определитель матрицы G меньше этого значения, то считается, что нет потока
constexpr float MIN_DETERMINANT = 1;

namespace
{
void create_image_pyramid_sizes(int width, int height, int min, std::vector<glm::ivec2>* level_dimensions)
{
        level_dimensions->clear();
        level_dimensions->emplace_back(width, height);
        while (true)
        {
                int new_width = (width + 1) / 2;
                int new_height = (height + 1) / 2;

                if (new_width < min)
                {
                        new_width = width;
                }
                if (new_height < min)
                {
                        new_height = height;
                }

                if (new_width == width && new_height == height)
                {
                        break;
                }

                level_dimensions->emplace_back(new_width, new_height);

                width = new_width;
                height = new_height;
        }

        // for (const glm::ivec2& v : *level_dimensions)
        //{
        //        LOG(to_string(v.x) + " x " + to_string(v.y));
        //}
}

class ImageR32F
{
        TextureR32F m_texture;
        GLuint64 m_image_write_handle;
        GLuint64 m_image_read_handle;
        GLuint64 m_texture_handle;
        int m_width;
        int m_height;

public:
        ImageR32F(int x, int y)
                : m_texture(x, y),
                  m_image_write_handle(m_texture.get_texture().get_image_resident_handle_write_only_R32F()),
                  m_image_read_handle(m_texture.get_texture().get_image_resident_handle_read_only_R32F()),
                  m_texture_handle(m_texture.get_texture().get_texture_resident_handle()),
                  m_width(x),
                  m_height(y)
        {
        }
        int get_width() const
        {
                return m_width;
        }
        int get_height() const
        {
                return m_height;
        }
        GLuint64 get_image_write_handle() const
        {
                return m_image_write_handle;
        }
        GLuint64 get_image_read_handle() const
        {
                return m_image_read_handle;
        }
        GLuint64 get_texture_handle() const
        {
                return m_texture_handle;
        }
};

void create_textures(const std::vector<glm::ivec2>& level_dimensions, std::vector<ImageR32F>* textures)
{
        textures->clear();
        for (const glm::ivec2& d : level_dimensions)
        {
                textures->emplace_back(d.x, d.y);
        }
}

void create_flow_buffers(const std::vector<glm::ivec2>& level_dimensions, std::vector<ShaderStorageBuffer>* buffers)
{
        buffers->clear();
        buffers->resize(level_dimensions.size());
        for (unsigned i = 0; i < level_dimensions.size(); ++i)
        {
                (*buffers)[i].create_dynamic_copy(level_dimensions[i].x * level_dimensions[i].y * sizeof(glm::vec2));
        }
}

void create_points_for_top_level(int width, int height, int distance, int* point_count_x, int* point_count_y,
                                 std::vector<glm::ivec2>* points)
{
        int size = distance + 1;
        *point_count_x = (width - 2 * distance + size - 1) / size;
        *point_count_y = (height - 2 * distance + size - 1) / size;

        int point_count = *point_count_x * *point_count_y;

        points->clear();
        points->resize(point_count);

        int index = 0;
        for (float y = distance; y < height - distance; y += size)
        {
                for (float x = distance; x < width - distance; x += size)
                {
                        (*points)[index++] = glm::ivec2(x, y);
                }
        }

        ASSERT(index == point_count);
}
}

class OpticalFlow::Impl final
{
        const int m_width, m_height;
        const int m_groups_x, m_groups_y;
        ComputeProgram m_comp_sobel;
        ComputeProgram m_comp_flow;
        ComputeProgram m_comp_downsample;
        ComputeProgram m_comp_grayscale;
        ComputeProgram m_comp_lines;
        GraphicsProgram m_draw_prog;
        GraphicsProgram m_draw_prog_debug;

        TextureRGBA32F m_texture_J;

        ShaderStorageBuffer m_top_points, m_top_points_flow, m_top_points_lines;
        int m_point_count_x, m_point_count_y;

        bool m_image_I_exists = false;
        bool m_flow_computed = false;
        double m_last_time = std::numeric_limits<double>::lowest();

        std::array<std::vector<ImageR32F>, 2> m_image_pyramid;
        std::vector<ImageR32F> m_image_pyramid_dx;
        std::vector<ImageR32F> m_image_pyramid_dy;
        std::vector<ShaderStorageBuffer> m_image_pyramid_flow;
        int m_i_index = 0;
        int m_j_index = 1;

        void build_image_pyramid(std::vector<ImageR32F>* pyramid)
        {
                // уровень 0 заполняется по исходному изображению
                m_comp_grayscale.set_uniform_handle("img_dst", (*pyramid)[0].get_image_write_handle());
                m_comp_grayscale.dispatch_compute(m_groups_x, m_groups_y, 1, GROUP_SIZE, GROUP_SIZE, 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

                // Каждый следующий уровень меньше предыдущего
                for (unsigned i = 1; i < pyramid->size(); ++i)
                {
                        const ImageR32F& img_big = (*pyramid)[i - 1];
                        const ImageR32F& img_small = (*pyramid)[i];

                        int k_x = (img_small.get_width() != img_big.get_width()) ? 2 : 1;
                        int k_y = (img_small.get_height() != img_big.get_height()) ? 2 : 1;

                        ASSERT(k_x > 1 || k_y > 1);

                        m_comp_downsample.set_uniform_handle("img_big", img_big.get_image_read_handle());
                        m_comp_downsample.set_uniform_handle("img_small", img_small.get_image_write_handle());
                        m_comp_downsample.set_uniform("k_x", k_x);
                        m_comp_downsample.set_uniform("k_y", k_y);

                        int groups_x = get_group_count(img_small.get_width(), GROUP_SIZE);
                        int groups_y = get_group_count(img_small.get_height(), GROUP_SIZE);

                        m_comp_downsample.dispatch_compute(groups_x, groups_y, 1, GROUP_SIZE, GROUP_SIZE, 1);
                        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
                }
        }

        void compute_dxdy(const std::vector<ImageR32F>& image_pyramid, const std::vector<ImageR32F>& image_pyramid_dx,
                          const std::vector<ImageR32F>& image_pyramid_dy)
        {
                ASSERT(image_pyramid.size() == image_pyramid_dx.size() && image_pyramid.size() == image_pyramid_dy.size());

                for (unsigned i = 0; i < image_pyramid.size(); ++i)
                {
                        m_comp_sobel.set_uniform_handle("img_I", image_pyramid[i].get_image_read_handle());
                        m_comp_sobel.set_uniform_handle("img_dx", image_pyramid_dx[i].get_image_write_handle());
                        m_comp_sobel.set_uniform_handle("img_dy", image_pyramid_dy[i].get_image_write_handle());

                        int groups_x = get_group_count(image_pyramid[i].get_width(), GROUP_SIZE);
                        int groups_y = get_group_count(image_pyramid[i].get_height(), GROUP_SIZE);

                        m_comp_sobel.dispatch_compute(groups_x, groups_y, 1, GROUP_SIZE, GROUP_SIZE, 1);
                }

                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }

        void compute_optical_flow(const std::vector<ImageR32F>& image_pyramid_I, const std::vector<ImageR32F>& image_pyramid_dx,
                                  const std::vector<ImageR32F>& image_pyramid_dy,
                                  const std::vector<ShaderStorageBuffer>& image_pyramid_flow,
                                  const std::vector<ImageR32F>& image_pyramid_J)
        {
                int image_pyramid_I_size = image_pyramid_I.size();

                for (int i = image_pyramid_I_size - 1; i >= 0; --i)
                {
                        int points_x, points_y;
                        if (i != 0)
                        {
                                // Если не самый верхний уровень, то расчёт для всех точек

                                m_comp_flow.set_uniform("all_points", 1);

                                image_pyramid_flow[i].bind(1);

                                points_x = image_pyramid_I[i].get_width();
                                points_y = image_pyramid_I[i].get_height();
                        }
                        else
                        {
                                // Если самый верхний уровень, то расчёт только для заданных точек для рисования на экране

                                m_comp_flow.set_uniform("all_points", 0);

                                m_top_points.bind(0);
                                m_top_points_flow.bind(1);

                                points_x = m_point_count_x;
                                points_y = m_point_count_y;
                        }

                        if (i != image_pyramid_I_size - 1)
                        {
                                // Если не самый нижний уровень, то в качестве приближения использовать поток,
                                // полученный на меньших изображениях

                                m_comp_flow.set_uniform("use_guess", 1);

                                m_comp_flow.set_uniform("guess_width", image_pyramid_I[i + 1].get_width());

                                image_pyramid_flow[i + 1].bind(2);

                                int kx = (image_pyramid_I[i + 1].get_width() != image_pyramid_I[i].get_width()) ? 2 : 1;
                                int ky = (image_pyramid_I[i + 1].get_height() != image_pyramid_I[i].get_height()) ? 2 : 1;
                                m_comp_flow.set_uniform("guess_kx", kx);
                                m_comp_flow.set_uniform("guess_ky", ky);
                        }
                        else
                        {
                                // Самый нижний уровень пирамиды, поэтому нет начального потока
                                m_comp_flow.set_uniform("use_guess", 0);
                        }

                        m_comp_flow.set_uniform("point_count_x", points_x);
                        m_comp_flow.set_uniform("point_count_y", points_y);

                        int groups_x = get_group_count(points_x, GROUP_SIZE);
                        int groups_y = get_group_count(points_y, GROUP_SIZE);

                        m_comp_flow.set_uniform_handle("img_dx", image_pyramid_dx[i].get_image_read_handle());
                        m_comp_flow.set_uniform_handle("img_dy", image_pyramid_dy[i].get_image_read_handle());
                        m_comp_flow.set_uniform_handle("img_I", image_pyramid_I[i].get_image_read_handle());
                        m_comp_flow.set_uniform_handle("tex_J", image_pyramid_J[i].get_texture_handle());

                        m_comp_flow.dispatch_compute(groups_x, groups_y, 1, GROUP_SIZE, GROUP_SIZE, 1);

                        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
                }
        }

        void create_flow_lines()
        {
                m_top_points.bind(0);
                m_top_points_flow.bind(1);
                m_top_points_lines.bind(2);

                int groups_x = get_group_count(m_point_count_x, GROUP_SIZE);
                int groups_y = get_group_count(m_point_count_y, GROUP_SIZE);

                m_comp_lines.dispatch_compute(groups_x, groups_y, 1, GROUP_SIZE, GROUP_SIZE, 1);

                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        void draw_lines()
        {
                m_top_points_lines.bind(0);

                m_draw_prog.draw_arrays(GL_POINTS, 0, m_point_count_x * m_point_count_y * 2);
                m_draw_prog.draw_arrays(GL_LINES, 0, m_point_count_x * m_point_count_y * 2);
        }

public:
        Impl(int width, int height, const glm::mat4& mtx)
                : m_width(width),
                  m_height(height),
                  m_groups_x(get_group_count(m_width, GROUP_SIZE)),
                  m_groups_y(get_group_count(m_height, GROUP_SIZE)),
                  m_comp_sobel(ComputeShader(sobel_compute_shader)),
                  m_comp_flow(ComputeShader(flow_compute_shader)),
                  m_comp_downsample(ComputeShader(downsample_compute_shader)),
                  m_comp_grayscale(ComputeShader(grayscale_compute_shader)),
                  m_comp_lines(ComputeShader(lines_compute_shader)),
                  m_draw_prog(VertexShader(vertex_shader), FragmentShader(fragment_shader)),
                  m_draw_prog_debug(VertexShader(vertex_debug_shader), FragmentShader(fragment_debug_shader)),
                  m_texture_J(m_width, m_height)
        {
                std::vector<glm::ivec2> level_dimensions;

                create_image_pyramid_sizes(m_width, m_height, BOTTOM_IMAGE_SIZE, &level_dimensions);

                create_textures(level_dimensions, &m_image_pyramid[m_i_index]);
                create_textures(level_dimensions, &m_image_pyramid[m_j_index]);
                create_textures(level_dimensions, &m_image_pyramid_dx);
                create_textures(level_dimensions, &m_image_pyramid_dy);

                create_flow_buffers(level_dimensions, &m_image_pyramid_flow);

                std::vector<glm::ivec2> top_points;
                create_points_for_top_level(m_width, m_height, POINT_DISTANCE, &m_point_count_x, &m_point_count_y, &top_points);

                m_top_points.load_dynamic_copy(top_points);
                m_top_points_flow.create_dynamic_copy(top_points.size() * sizeof(glm::vec2));
                m_top_points_lines.create_dynamic_copy(top_points.size() * 2 * sizeof(glm::ivec2));

                m_comp_grayscale.set_uniform_handle("img_src", m_texture_J.get_image_resident_handle_read_only());

                m_comp_lines.set_uniform("point_count_x", m_point_count_x);
                m_comp_lines.set_uniform("point_count_y", m_point_count_y);

                m_comp_flow.set_uniform("RADIUS", RADIUS);
                m_comp_flow.set_uniform("ITERATION_COUNT", ITERATION_COUNT);
                m_comp_flow.set_uniform("STOP_MOVE_SQUARE", STOP_MOVE_SQUARE);
                m_comp_flow.set_uniform("MIN_DETERMINANT", MIN_DETERMINANT);

                m_draw_prog.set_uniform("mvpMatrix", mtx);
        }

        void reset()
        {
                m_last_time = std::numeric_limits<double>::lowest();
                m_image_I_exists = false;
                m_flow_computed = false;
        }

        void copy_image()
        {
                m_texture_J.copy_texture_sub_image();
        }

        void draw()
        {
#if 0
                double current_time = get_time_seconds();
                if (current_time - m_last_time < COMPUTE_INTERVAL_SECONDS)
                {
                        if (m_flow_computed)
                        {
                                draw_lines();
                        }
                        return;
                }
                m_last_time = current_time;
#endif

                // Обозначения: I и i - предыдущее изображение, J и j - следующее изображение

                std::swap(m_i_index, m_j_index);

                build_image_pyramid(&m_image_pyramid[m_j_index]);

                if (!m_image_I_exists)
                {
                        m_image_I_exists = true;
                        return;
                }

                compute_dxdy(m_image_pyramid[m_i_index], m_image_pyramid_dx, m_image_pyramid_dy);

                compute_optical_flow(m_image_pyramid[m_i_index], m_image_pyramid_dx, m_image_pyramid_dy, m_image_pyramid_flow,
                                     m_image_pyramid[m_j_index]);

#if 0
                // m_draw_prog_image.set_uniform_handle("tex", m_image_pyramid_dx[0].get_texture_handle());
                m_draw_prog_debug.set_uniform_handle("tex", m_image_pyramid[m_i_index][0].get_texture_handle());
                m_draw_prog_debug.draw_arrays(GL_TRIANGLE_STRIP, 0, 4);
#else

                create_flow_lines();

                draw_lines();

                m_flow_computed = true;
#endif
        }
};

OpticalFlow::OpticalFlow(int width, int height, const glm::mat4& mtx) : m_impl(std::make_unique<Impl>(width, height, mtx))
{
}

OpticalFlow::~OpticalFlow() = default;

void OpticalFlow::reset()
{
        m_impl->reset();
}

void OpticalFlow::copy_image()
{
        m_impl->copy_image();
}

void OpticalFlow::draw()
{
        m_impl->draw();
}
