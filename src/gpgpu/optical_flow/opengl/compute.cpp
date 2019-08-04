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

#include "compute.h"

#include "compute_memory.h"

#include "com/error.h"
#include "com/log.h"
#include "com/math.h"
#include "com/print.h"
#include "gpgpu/com/groups.h"
#include "graphics/opengl/shader.h"

#include <array>

// clang-format off
constexpr const char sobel_shader[]
{
#include "of_sobel.comp.str"
};
constexpr const char flow_shader[]
{
#include "of_flow.comp.str"
};
constexpr const char downsample_shader[]
{
#include "of_downsample.comp.str"
};
constexpr const char grayscale_shader[]
{
#include "of_grayscale.comp.str"
};
// clang-format on

// Размер по X и по Y группы потоков вычислительных шейдеров
constexpr int GROUP_SIZE = 16;
// Минимальный размер изображения для пирамиды изображений
constexpr int BOTTOM_IMAGE_SIZE = 16;

// Параметры алгоритма для передачи в вычислительный шейдер
// Радиус окрестности точки
constexpr int RADIUS = 6;
// Максимальное количество итераций
constexpr int ITERATION_COUNT = 10;
// Если на итерации квадрат потока меньше этого значения, то выход из цикла
constexpr float STOP_MOVE_SQUARE = square(1e-3f);
// Если определитель матрицы G меньше этого значения, то считается, что нет потока
constexpr float MIN_DETERMINANT = 1;

namespace impl = gpgpu_optical_flow_compute_opengl_implementation;

namespace
{
std::string grayscale_source()
{
        std::string s;
        s += "const uint GROUP_SIZE = " + to_string(GROUP_SIZE) + ";\n";
        return s + grayscale_shader;
}

std::string downsample_source()
{
        std::string s;
        s += "const uint GROUP_SIZE = " + to_string(GROUP_SIZE) + ";\n";
        return s + downsample_shader;
}

std::string sobel_source()
{
        std::string s;
        s += "const uint GROUP_SIZE = " + to_string(GROUP_SIZE) + ";\n";
        return s + sobel_shader;
}

std::string flow_source()
{
        std::string s;
        s += "const uint GROUP_SIZE = " + to_string(GROUP_SIZE) + ";\n";
        s += "const int RADIUS = " + to_string(RADIUS) + ";\n";
        s += "const int ITERATION_COUNT = " + to_string(ITERATION_COUNT) + ";\n";
        s += "const float STOP_MOVE_SQUARE = " + to_string(STOP_MOVE_SQUARE) + ";\n";
        s += "const float MIN_DETERMINANT = " + to_string(MIN_DETERMINANT) + ";\n";
        return s + flow_shader;
}

std::vector<vec2i> pyramid_sizes(int width, int height, int min_size)
{
        std::vector<vec2i> sizes;

        sizes.emplace_back(width, height);

        while (true)
        {
                int new_width = (width + 1) / 2;
                int new_height = (height + 1) / 2;

                if (new_width < min_size)
                {
                        new_width = width;
                }
                if (new_height < min_size)
                {
                        new_height = height;
                }

                if (new_width == width && new_height == height)
                {
                        break;
                }

                sizes.emplace_back(new_width, new_height);

                width = new_width;
                height = new_height;
        }

#if 0
        for (const vec2i& v : sizes)
        {
                LOG(to_string(v[0]) + " x " + to_string(v[1]));
        }
#endif

        return sizes;
}

std::vector<opengl::TextureR32F> create_images(const std::vector<vec2i>& sizes)
{
        std::vector<opengl::TextureR32F> images;
        images.reserve(sizes.size());
        for (const vec2i& s : sizes)
        {
                images.emplace_back(s[0], s[1]);
        }
        return images;
}

std::vector<opengl::StorageBuffer> create_flow_buffers(const std::vector<vec2i>& sizes)
{
        std::vector<opengl::StorageBuffer> buffers;
        if (sizes.size() <= 1)
        {
                return {};
        }
        buffers.reserve(sizes.size() - 1);
        for (size_t i = 1; i < sizes.size(); ++i)
        {
                buffers.emplace_back(sizes[i][0] * sizes[i][1] * sizeof(vec2f));
        }
        return buffers;
}

std::array<impl::GrayscaleMemory, 2> create_grayscale_memory(const opengl::TextureRGBA32F& source_image,
                                                             const std::array<std::vector<opengl::TextureR32F>, 2>& images)
{
        return {impl::GrayscaleMemory(source_image, images[0][0]), impl::GrayscaleMemory(source_image, images[1][0])};
}

vec2i create_grayscale_groups(const std::vector<vec2i>& sizes)
{
        int x = group_count(sizes[0][0], GROUP_SIZE);
        int y = group_count(sizes[0][1], GROUP_SIZE);

        return {x, y};
}

std::array<std::vector<impl::DownsampleMemory>, 2> create_downsample_memory(
        const std::array<std::vector<opengl::TextureR32F>, 2>& images)
{
        ASSERT(images[0].size() == images[1].size());

        std::array<std::vector<impl::DownsampleMemory>, 2> downsample_images;

        for (unsigned i = 1; i < images[0].size(); ++i)
        {
                downsample_images[0].emplace_back(images[0][i - 1], images[0][i]);
                downsample_images[1].emplace_back(images[1][i - 1], images[1][i]);
        }

        return downsample_images;
}

std::vector<vec2i> create_downsample_groups(const std::vector<vec2i>& sizes)
{
        std::vector<vec2i> groups;

        for (unsigned i = 1; i < sizes.size(); ++i)
        {
                int x = group_count(sizes[i][0], GROUP_SIZE);
                int y = group_count(sizes[i][1], GROUP_SIZE);
                groups.push_back({x, y});
        }

        return groups;
}

std::array<std::vector<impl::SobelMemory>, 2> create_sobel_memory(const std::array<std::vector<opengl::TextureR32F>, 2>& images,
                                                                  const std::vector<opengl::TextureR32F>& dx,
                                                                  const std::vector<opengl::TextureR32F>& dy)
{
        ASSERT(images[0].size() == images[1].size());
        ASSERT(images[0].size() == dx.size());
        ASSERT(images[0].size() == dy.size());

        std::array<std::vector<impl::SobelMemory>, 2> sobel_images;

        for (size_t i = 0; i < images[0].size(); ++i)
        {
                sobel_images[0].emplace_back(images[0][i], dx[i], dy[i]);
                sobel_images[1].emplace_back(images[1][i], dx[i], dy[i]);
        }

        return sobel_images;
}

std::vector<vec2i> create_sobel_groups(const std::vector<vec2i>& sizes)
{
        std::vector<vec2i> groups;

        for (size_t i = 0; i < sizes.size(); ++i)
        {
                int x = group_count(sizes[i][0], GROUP_SIZE);
                int y = group_count(sizes[i][1], GROUP_SIZE);
                groups.push_back({x, y});
        }

        return groups;
}

std::vector<impl::FlowDataMemory> create_flow_data_memory(const std::vector<vec2i>& sizes,
                                                          const std::vector<opengl::StorageBuffer>& flow_buffers, int top_x,
                                                          int top_y, const opengl::StorageBuffer& top_points,
                                                          const opengl::StorageBuffer& top_flow)
{
        ASSERT(flow_buffers.size() + 1 == sizes.size());
        auto flow_index = [&](size_t i) {
                ASSERT(i > 0 && i <= flow_buffers.size());
                return i - 1; // буферы начинаются с уровня 1
        };

        std::vector<impl::FlowDataMemory> flow_data(sizes.size());

        for (size_t i = 0; i < flow_data.size(); ++i)
        {
                impl::FlowDataMemory::Data data;

                const bool top = (i == 0);
                const bool bottom = (i + 1 == flow_data.size());

                if (!top)
                {
                        // Не самый верхний уровень, поэтому расчёт для всех точек
                        flow_data[i].set_top_points(nullptr);
                        flow_data[i].set_flow(&flow_buffers[flow_index(i)]);
                        data.use_all_points = 1;
                        data.point_count_x = sizes[i][0];
                        data.point_count_y = sizes[i][1];
                }
                else
                {
                        // Самый верхний уровень, поэтому расчёт только для заданных
                        // точек для рисования на экране
                        flow_data[i].set_top_points(&top_points);
                        flow_data[i].set_flow(&top_flow);
                        data.use_all_points = 0;
                        data.point_count_x = top_x;
                        data.point_count_y = top_y;
                }

                if (!bottom)
                {
                        // Не самый нижний уровень, поэтому в качестве приближения
                        // использовать поток, полученный на меньших изображениях
                        int i_prev = i + 1;
                        data.use_guess = 1;
                        data.guess_kx = (sizes[i_prev][0] != sizes[i][0]) ? 2 : 1;
                        data.guess_ky = (sizes[i_prev][1] != sizes[i][1]) ? 2 : 1;
                        data.guess_width = sizes[i_prev][0];
                        flow_data[i].set_flow_guess(&flow_buffers[flow_index(i_prev)]);
                }
                else
                {
                        // Самый нижний уровень пирамиды, поэтому нет приближения
                        flow_data[i].set_flow_guess(nullptr);
                        data.use_guess = 0;
                }

                flow_data[i].set_data(data);
        }

        return flow_data;
}

std::array<std::vector<impl::FlowImagesMemory>, 2> create_flow_images_memory(
        const std::array<std::vector<opengl::TextureR32F>, 2>& images, const std::vector<opengl::TextureR32F>& dx,
        const std::vector<opengl::TextureR32F>& dy)
{
        ASSERT(images[0].size() == images[1].size());
        ASSERT(images[0].size() == dx.size());
        ASSERT(images[0].size() == dy.size());

        std::array<std::vector<impl::FlowImagesMemory>, 2> flow_images;

        for (size_t i = 0; i < images[0].size(); ++i)
        {
                flow_images[0].emplace_back(dx[i], dy[i], images[0][i], images[1][i]);
                flow_images[1].emplace_back(dx[i], dy[i], images[1][i], images[0][i]);
        }

        return flow_images;
}

std::vector<vec2i> create_flow_groups(const std::vector<vec2i>& sizes, int top_x, int top_y)
{
        std::vector<vec2i> groups;

        groups.push_back({group_count(top_x, GROUP_SIZE), group_count(top_y, GROUP_SIZE)});

        for (size_t i = 1; i < sizes.size(); ++i)
        {
                int x = group_count(sizes[i][0], GROUP_SIZE);
                int y = group_count(sizes[i][1], GROUP_SIZE);
                groups.push_back({x, y});
        }

        return groups;
}

class Impl final : public gpgpu_opengl::OpticalFlowCompute
{
        std::array<std::vector<opengl::TextureR32F>, 2> m_images;
        std::vector<opengl::TextureR32F> m_dx;
        std::vector<opengl::TextureR32F> m_dy;
        std::vector<opengl::StorageBuffer> m_flow_buffers;

        std::array<impl::GrayscaleMemory, 2> m_grayscale_memory;
        vec2i m_grayscale_groups;
        opengl::ComputeProgram m_grayscale_compute;

        std::array<std::vector<impl::DownsampleMemory>, 2> m_downsample_memory;
        std::vector<vec2i> m_downsample_groups;
        opengl::ComputeProgram m_downsample_compute;

        std::array<std::vector<impl::SobelMemory>, 2> m_sobel_memory;
        std::vector<vec2i> m_sobel_groups;
        opengl::ComputeProgram m_sobel_compute;

        std::vector<impl::FlowDataMemory> m_flow_data_memory;
        std::array<std::vector<impl::FlowImagesMemory>, 2> m_flow_images_memory;
        std::vector<vec2i> m_flow_groups;
        opengl::ComputeProgram m_flow_compute;

        int m_i_index = -1;

        void build_image_pyramid(int index) const
        {
                ASSERT(index == 0 || index == 1);
                ASSERT(m_downsample_memory[index].size() == m_downsample_groups.size());

                // Уровень 0 заполняется по исходному изображению
                m_grayscale_memory[index].bind();
                m_grayscale_compute.dispatch_compute(m_grayscale_groups[0], m_grayscale_groups[1], 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

                // Каждый следующий уровень меньше предыдущего
                for (unsigned i = 0; i < m_downsample_groups.size(); ++i)
                {
                        m_downsample_memory[index][i].bind();
                        m_downsample_compute.dispatch_compute(m_downsample_groups[i][0], m_downsample_groups[i][1], 1);
                        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
                }
        }

        void compute_dxdy(int index) const
        {
                ASSERT(index == 0 || index == 1);
                ASSERT(m_sobel_memory[index].size() == m_sobel_groups.size());

                for (unsigned i = 0; i < m_sobel_groups.size(); ++i)
                {
                        m_sobel_memory[index][i].bind();
                        m_sobel_compute.dispatch_compute(m_sobel_groups[i][0], m_sobel_groups[i][1], 1);
                        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
                }
        }

        void compute_optical_flow(int index) const
        {
                ASSERT(index == 0 || index == 1);
                ASSERT(m_flow_data_memory.size() == m_flow_images_memory[index].size());
                ASSERT(m_flow_data_memory.size() == m_flow_groups.size());

                for (int i = static_cast<int>(m_flow_groups.size()) - 1; i >= 0; --i)
                {
                        m_flow_data_memory[i].bind();
                        m_flow_images_memory[index][i].bind();
                        m_flow_compute.dispatch_compute(m_flow_groups[i][0], m_flow_groups[i][1], 1);
                        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
                }
        }

        void reset() override
        {
                m_i_index = -1;
        }

        void exec() override
        {
                ASSERT(m_i_index == -1 || m_i_index == 0 || m_i_index == 1);

                if (m_i_index < 0)
                {
                        m_i_index = 0;
                        build_image_pyramid(m_i_index);
                }
                else
                {
                        m_i_index = 1 - m_i_index;
                }

                // i — предыдущее изображение, 1-i — текущее изображение

                build_image_pyramid(1 - m_i_index);

                compute_dxdy(m_i_index);

                compute_optical_flow(m_i_index);
        }

        GLuint64 image_pyramid_dx_texture() const override
        {
                return m_dx[0].texture().texture_resident_handle();
        }
        GLuint64 image_pyramid_texture() const override
        {
                return m_images[m_i_index][0].texture().texture_resident_handle();
        }

        Impl(const std::vector<vec2i>& sizes, const opengl::TextureRGBA32F& source_image, int top_x, int top_y,
             const opengl::StorageBuffer& top_points, const opengl::StorageBuffer& top_flow)
                : m_images({create_images(sizes), create_images(sizes)}),
                  m_dx(create_images(sizes)),
                  m_dy(create_images(sizes)),
                  m_flow_buffers(create_flow_buffers(sizes)),
                  //
                  m_grayscale_memory(create_grayscale_memory(source_image, m_images)),
                  m_grayscale_groups(create_grayscale_groups(sizes)),
                  m_grayscale_compute(opengl::ComputeShader(grayscale_source())),
                  //
                  m_downsample_memory(create_downsample_memory(m_images)),
                  m_downsample_groups(create_downsample_groups(sizes)),
                  m_downsample_compute(opengl::ComputeShader(downsample_source())),
                  //
                  m_sobel_memory(create_sobel_memory(m_images, m_dx, m_dy)),
                  m_sobel_groups(create_sobel_groups(sizes)),
                  m_sobel_compute(opengl::ComputeShader(sobel_source())),
                  //
                  m_flow_data_memory(create_flow_data_memory(sizes, m_flow_buffers, top_x, top_y, top_points, top_flow)),
                  m_flow_images_memory(create_flow_images_memory(m_images, m_dx, m_dy)),
                  m_flow_groups(create_flow_groups(sizes, top_x, top_y)),
                  m_flow_compute(opengl::ComputeShader(flow_source()))
        {
        }

public:
        Impl(int width, int height, const opengl::TextureRGBA32F& source_image, int top_x, int top_y,
             const opengl::StorageBuffer& top_points, const opengl::StorageBuffer& top_flow)
                : Impl(pyramid_sizes(width, height, BOTTOM_IMAGE_SIZE), source_image, top_x, top_y, top_points, top_flow)
        {
        }
};
}

namespace gpgpu_opengl
{
std::unique_ptr<OpticalFlowCompute> create_optical_flow_compute(int width, int height, const opengl::TextureRGBA32F& source_image,
                                                                int top_x, int top_y, const opengl::StorageBuffer& top_points,
                                                                const opengl::StorageBuffer& top_flow)
{
        return std::make_unique<Impl>(width, height, source_image, top_x, top_y, top_points, top_flow);
}
}
