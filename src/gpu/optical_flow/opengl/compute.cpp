/*
Copyright (C) 2017-2020 Topological Manifold

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

#if defined(OPENGL_FOUND)

#include "compute.h"

#include "compute_memory.h"
#include "compute_program.h"

#include "com/error.h"
#include "com/groups.h"
#include "com/log.h"
#include "com/math.h"
#include "com/print.h"
#include "gpu/optical_flow/com/compute.h"

#include <array>

constexpr GLenum IMAGE_FORMAT = GL_R32F;

namespace gpu_opengl
{
namespace
{
std::vector<opengl::Texture> create_images(const std::vector<vec2i>& sizes)
{
        std::vector<opengl::Texture> images;
        images.reserve(sizes.size());
        for (const vec2i& s : sizes)
        {
                images.emplace_back(IMAGE_FORMAT, s[0], s[1]);
        }
        return images;
}

std::vector<opengl::Buffer> create_flow_buffers(const std::vector<vec2i>& sizes)
{
        std::vector<opengl::Buffer> buffers;
        if (sizes.size() <= 1)
        {
                return {};
        }
        buffers.reserve(sizes.size() - 1);
        for (size_t i = 1; i < sizes.size(); ++i)
        {
                buffers.emplace_back(sizes[i][0] * sizes[i][1] * sizeof(vec2f), 0);
        }
        return buffers;
}

std::array<OpticalFlowGrayscaleMemory, 2> create_grayscale_memory(
        const opengl::Texture& source,
        const std::array<std::vector<opengl::Texture>, 2>& images)
{
        return {OpticalFlowGrayscaleMemory(source, images[0][0]), OpticalFlowGrayscaleMemory(source, images[1][0])};
}

std::array<std::vector<OpticalFlowDownsampleMemory>, 2> create_downsample_memory(
        const std::array<std::vector<opengl::Texture>, 2>& images)
{
        ASSERT(images[0].size() == images[1].size());

        std::array<std::vector<OpticalFlowDownsampleMemory>, 2> downsample_images;

        for (unsigned i = 1; i < images[0].size(); ++i)
        {
                downsample_images[0].emplace_back(images[0][i - 1], images[0][i]);
                downsample_images[1].emplace_back(images[1][i - 1], images[1][i]);
        }

        return downsample_images;
}

std::array<std::vector<OpticalFlowSobelMemory>, 2> create_sobel_memory(
        const std::array<std::vector<opengl::Texture>, 2>& images,
        const std::vector<opengl::Texture>& dx,
        const std::vector<opengl::Texture>& dy)
{
        ASSERT(images[0].size() == images[1].size());
        ASSERT(images[0].size() == dx.size());
        ASSERT(images[0].size() == dy.size());

        std::array<std::vector<OpticalFlowSobelMemory>, 2> sobel_images;

        for (size_t i = 0; i < images[0].size(); ++i)
        {
                sobel_images[0].emplace_back(images[0][i], dx[i], dy[i]);
                sobel_images[1].emplace_back(images[1][i], dx[i], dy[i]);
        }

        return sobel_images;
}

std::vector<OpticalFlowDataMemory> create_flow_data_memory(
        const std::vector<vec2i>& sizes,
        const std::vector<opengl::Buffer>& flow_buffers,
        int top_point_count_x,
        int top_point_count_y,
        const opengl::Buffer& top_points,
        const opengl::Buffer& top_flow)
{
        ASSERT(flow_buffers.size() + 1 == sizes.size());
        auto flow_index = [&](size_t i) {
                ASSERT(i > 0 && i <= flow_buffers.size());
                return i - 1; // буферы начинаются с уровня 1
        };

        std::vector<OpticalFlowDataMemory> flow_data(sizes.size());

        for (size_t i = 0; i < flow_data.size(); ++i)
        {
                OpticalFlowDataMemory::Data data;

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
                        data.point_count_x = top_point_count_x;
                        data.point_count_y = top_point_count_y;
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

std::array<std::vector<OpticalFlowImagesMemory>, 2> create_flow_images_memory(
        const std::array<std::vector<opengl::Texture>, 2>& images,
        const std::vector<opengl::Texture>& dx,
        const std::vector<opengl::Texture>& dy)
{
        ASSERT(images[0].size() == images[1].size());
        ASSERT(images[0].size() == dx.size());
        ASSERT(images[0].size() == dy.size());

        std::array<std::vector<OpticalFlowImagesMemory>, 2> flow_images;

        for (size_t i = 0; i < images[0].size(); ++i)
        {
                flow_images[0].emplace_back(dx[i], dy[i], images[0][i], images[1][i]);
                flow_images[1].emplace_back(dx[i], dy[i], images[1][i], images[0][i]);
        }

        return flow_images;
}

class Impl final : public OpticalFlowCompute
{
        std::array<std::vector<opengl::Texture>, 2> m_images;
        std::vector<opengl::Texture> m_dx;
        std::vector<opengl::Texture> m_dy;
        std::vector<opengl::Buffer> m_flow_buffers;

        std::array<OpticalFlowGrayscaleMemory, 2> m_grayscale_memory;
        vec2i m_grayscale_groups;
        OpticalFlowGrayscaleProgram m_grayscale_compute;

        std::array<std::vector<OpticalFlowDownsampleMemory>, 2> m_downsample_memory;
        std::vector<vec2i> m_downsample_groups;
        OpticalFlowDownsampleProgram m_downsample_compute;

        std::array<std::vector<OpticalFlowSobelMemory>, 2> m_sobel_memory;
        std::vector<vec2i> m_sobel_groups;
        OpticalFlowSobelProgram m_sobel_compute;

        std::vector<OpticalFlowDataMemory> m_flow_data_memory;
        std::array<std::vector<OpticalFlowImagesMemory>, 2> m_flow_images_memory;
        std::vector<vec2i> m_flow_groups;
        OpticalFlowFlowProgram m_flow_compute;

        int m_i_index = -1;

        void build_image_pyramid(int index) const
        {
                ASSERT(index == 0 || index == 1);
                ASSERT(m_downsample_memory[index].size() == m_downsample_groups.size());

                // Уровень 0 заполняется по исходному изображению
                m_grayscale_compute.exec(m_grayscale_groups, m_grayscale_memory[index]);

                // Каждый следующий уровень меньше предыдущего
                for (unsigned i = 0; i < m_downsample_groups.size(); ++i)
                {
                        m_downsample_compute.exec(m_downsample_groups[i], m_downsample_memory[index][i]);
                }
        }

        void compute_dxdy(int index) const
        {
                ASSERT(index == 0 || index == 1);
                ASSERT(m_sobel_memory[index].size() == m_sobel_groups.size());

                for (unsigned i = 0; i < m_sobel_groups.size(); ++i)
                {
                        m_sobel_compute.exec(m_sobel_groups[i], m_sobel_memory[index][i]);
                }
        }

        void compute_optical_flow(int index) const
        {
                ASSERT(index == 0 || index == 1);
                ASSERT(m_flow_data_memory.size() == m_flow_images_memory[index].size());
                ASSERT(m_flow_data_memory.size() == m_flow_groups.size());

                for (int i = static_cast<int>(m_flow_groups.size()) - 1; i >= 0; --i)
                {
                        m_flow_compute.exec(m_flow_groups[i], m_flow_data_memory[i], m_flow_images_memory[index][i]);
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
                return m_dx[0].texture_handle();
        }
        GLuint64 image_pyramid_texture() const override
        {
                return m_images[m_i_index][0].texture_handle();
        }

        Impl(const std::vector<vec2i>& sizes,
             const opengl::Texture& source,
             unsigned x,
             unsigned y,
             unsigned width,
             unsigned height,
             unsigned top_point_count_x,
             unsigned top_point_count_y,
             const opengl::Buffer& top_points,
             const opengl::Buffer& top_flow)
                : m_images({create_images(sizes), create_images(sizes)}),
                  m_dx(create_images(sizes)),
                  m_dy(create_images(sizes)),
                  m_flow_buffers(create_flow_buffers(sizes)),
                  //
                  m_grayscale_memory(create_grayscale_memory(source, m_images)),
                  m_grayscale_groups(optical_flow_grayscale_groups(OPTICAL_FLOW_GROUP_SIZE, sizes)),
                  m_grayscale_compute(OPTICAL_FLOW_GROUP_SIZE, x, y, width, height),
                  //
                  m_downsample_memory(create_downsample_memory(m_images)),
                  m_downsample_groups(optical_flow_downsample_groups(OPTICAL_FLOW_GROUP_SIZE, sizes)),
                  m_downsample_compute(OPTICAL_FLOW_GROUP_SIZE),
                  //
                  m_sobel_memory(create_sobel_memory(m_images, m_dx, m_dy)),
                  m_sobel_groups(optical_flow_sobel_groups(OPTICAL_FLOW_GROUP_SIZE, sizes)),
                  m_sobel_compute(OPTICAL_FLOW_GROUP_SIZE),
                  //
                  m_flow_data_memory(create_flow_data_memory(
                          sizes,
                          m_flow_buffers,
                          top_point_count_x,
                          top_point_count_y,
                          top_points,
                          top_flow)),
                  m_flow_images_memory(create_flow_images_memory(m_images, m_dx, m_dy)),
                  m_flow_groups(optical_flow_flow_groups(
                          OPTICAL_FLOW_GROUP_SIZE,
                          sizes,
                          top_point_count_x,
                          top_point_count_y)),
                  m_flow_compute(
                          OPTICAL_FLOW_GROUP_SIZE,
                          OPTICAL_FLOW_RADIUS,
                          OPTICAL_FLOW_ITERATION_COUNT,
                          OPTICAL_FLOW_STOP_MOVE_SQUARE,
                          OPTICAL_FLOW_MIN_DETERMINANT)
        {
                ASSERT(width > 0 && height > 0);
                ASSERT(x + width <= static_cast<unsigned>(source.width()));
                ASSERT(y + height <= static_cast<unsigned>(source.height()));
        }

public:
        Impl(const opengl::Texture& source,
             unsigned x,
             unsigned y,
             unsigned width,
             unsigned height,
             unsigned top_point_count_x,
             unsigned top_point_count_y,
             const opengl::Buffer& top_points,
             const opengl::Buffer& top_flow)
                : Impl(optical_flow_pyramid_sizes(source.width(), source.height(), OPTICAL_FLOW_BOTTOM_IMAGE_SIZE),
                       source,
                       x,
                       y,
                       width,
                       height,
                       top_point_count_x,
                       top_point_count_y,
                       top_points,
                       top_flow)
        {
        }
};
}

std::unique_ptr<OpticalFlowCompute> create_optical_flow_compute(
        const opengl::Texture& source,
        unsigned x,
        unsigned y,
        unsigned width,
        unsigned height,
        unsigned top_point_count_x,
        unsigned top_point_count_y,
        const opengl::Buffer& top_points,
        const opengl::Buffer& top_flow)
{
        return std::make_unique<Impl>(
                source, x, y, width, height, top_point_count_x, top_point_count_y, top_points, top_flow);
}
}

#endif
