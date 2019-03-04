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

#include "opengl_of_compute.h"

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

class ImageR32F final
{
        opengl::TextureR32F m_texture;
        GLuint64 m_image_write_handle;
        GLuint64 m_image_read_handle;
        GLuint64 m_texture_handle;
        int m_width;
        int m_height;

public:
        ImageR32F(int x, int y)
                : m_texture(x, y),
                  m_image_write_handle(m_texture.image_resident_handle_write_only()),
                  m_image_read_handle(m_texture.image_resident_handle_read_only()),
                  m_texture_handle(m_texture.texture().texture_resident_handle()),
                  m_width(x),
                  m_height(y)
        {
        }

        int width() const
        {
                return m_width;
        }

        int height() const
        {
                return m_height;
        }

        GLuint64 image_write_handle() const
        {
                return m_image_write_handle;
        }

        GLuint64 image_read_handle() const
        {
                return m_image_read_handle;
        }

        GLuint64 texture_handle() const
        {
                return m_texture_handle;
        }
};

class Pyramid final
{
        std::array<std::vector<ImageR32F>, 2> m_images;
        std::vector<ImageR32F> m_dx;
        std::vector<ImageR32F> m_dy;
        std::vector<opengl::StorageBuffer> m_flow;

        static std::vector<ImageR32F> create_images(const std::vector<vec2i>& sizes)
        {
                std::vector<ImageR32F> images;
                images.reserve(sizes.size());
                for (const vec2i& s : sizes)
                {
                        images.emplace_back(s[0], s[1]);
                }
                return images;
        }

        static std::vector<opengl::StorageBuffer> create_buffers(const std::vector<vec2i>& sizes)
        {
                std::vector<opengl::StorageBuffer> buffers;
                buffers.reserve(sizes.size());
                for (const vec2i& s : sizes)
                {
                        buffers.emplace_back(s[0] * s[1] * sizeof(vec2f));
                }
                return buffers;
        }

public:
        Pyramid(const std::vector<vec2i>& sizes)
                : m_images{create_images(sizes), create_images(sizes)},
                  m_dx(create_images(sizes)),
                  m_dy(create_images(sizes)),
                  m_flow(create_buffers(sizes))
        {
        }

        const std::vector<ImageR32F>& images(unsigned i) const
        {
                ASSERT(i < 2);
                return m_images[i];
        }

        const std::vector<ImageR32F>& dx() const
        {
                return m_dx;
        }

        const std::vector<ImageR32F>& dy() const
        {
                return m_dy;
        }

        const std::vector<opengl::StorageBuffer>& flow() const
        {
                return m_flow;
        }

        int width(size_t i) const
        {
                ASSERT(i < m_images[0].size());
                ASSERT(m_images[0].size() == m_images[1].size());
                ASSERT(m_images[0][i].width() == m_images[1][i].width());

                return m_images[0][i].width();
        }

        int height(size_t i) const
        {
                ASSERT(i < m_images[0].size());
                ASSERT(m_images[0].size() == m_images[1].size());
                ASSERT(m_images[0][i].height() == m_images[1][i].height());

                return m_images[0][i].height();
        }

        size_t size() const
        {
                ASSERT(m_images[0].size() == m_images[1].size());

                return m_images[0].size();
        }
};

class GrayscaleMemory final
{
        static constexpr int IMAGES_BINDING = 0;

        opengl::UniformBuffer m_buffer;

        struct Images
        {
                GLuint64 image_src;
                alignas(16) GLuint64 image_dst;
        };

public:
        GrayscaleMemory(const opengl::TextureRGBA32F& image_src, const ImageR32F& image_dst) : m_buffer(sizeof(Images))
        {
                Images images;

                images.image_src = image_src.image_resident_handle_read_only();
                images.image_dst = image_dst.image_write_handle();

                m_buffer.copy(images);
        }

        void bind() const
        {
                m_buffer.bind(IMAGES_BINDING);
        }
};

class DownsampleMemory final
{
        static constexpr int IMAGES_BINDING = 0;

        opengl::UniformBuffer m_buffer;

        struct Images
        {
                GLuint64 image_big;
                alignas(16) GLuint64 image_small;
        };

public:
        DownsampleMemory(const ImageR32F& image_big, const ImageR32F& image_small) : m_buffer(sizeof(Images))
        {
                Images images;

                images.image_big = image_big.image_read_handle();
                images.image_small = image_small.image_write_handle();

                m_buffer.copy(images);
        }

        void bind() const
        {
                m_buffer.bind(IMAGES_BINDING);
        }
};

class SobelMemory final
{
        static constexpr int IMAGES_BINDING = 0;

        opengl::UniformBuffer m_buffer;

        struct Images
        {
                GLuint64 image_i;
                alignas(16) GLuint64 image_dx;
                alignas(16) GLuint64 image_dy;
        };

public:
        SobelMemory(const ImageR32F& image_i, const ImageR32F& image_dx, const ImageR32F& image_dy) : m_buffer(sizeof(Images))
        {
                Images images;

                images.image_i = image_i.image_read_handle();
                images.image_dx = image_dx.image_write_handle();
                images.image_dy = image_dy.image_write_handle();

                m_buffer.copy(images);
        }

        void bind() const
        {
                m_buffer.bind(IMAGES_BINDING);
        }
};

class FlowDataMemory final
{
        static constexpr int POINTS_BINDING = 0;
        static constexpr int POINTS_FLOW_BINDING = 1;
        static constexpr int POINTS_FLOW_GUESS_BINDING = 2;
        static constexpr int DATA_BINDING = 3;

        const opengl::StorageBuffer* m_top_points = nullptr;
        const opengl::StorageBuffer* m_flow = nullptr;
        const opengl::StorageBuffer* m_flow_guess = nullptr;

        opengl::UniformBuffer m_buffer;

        int m_point_count_x = -1;
        int m_point_count_y = -1;

public:
        struct Data
        {
                GLint point_count_x;
                GLint point_count_y;
                GLuint use_all_points;
                GLuint use_guess;
                GLint guess_kx;
                GLint guess_ky;
                GLint guess_width;
        };

        FlowDataMemory() : m_buffer(sizeof(Data))
        {
        }

        void set_top_points(const opengl::StorageBuffer* top_points)
        {
                m_top_points = top_points;
        }

        void set_flow_guess(const opengl::StorageBuffer* flow_guess)
        {
                m_flow_guess = flow_guess;
        }

        void set_flow(const opengl::StorageBuffer* flow)
        {
                m_flow = flow;
        }

        void set_data(const Data& data)
        {
                m_buffer.copy(data);
                m_point_count_x = data.point_count_x;
                m_point_count_y = data.point_count_y;
        }

        int point_count_x() const
        {
                return m_point_count_x;
        }

        int point_count_y() const
        {
                return m_point_count_y;
        }

        void bind() const
        {
                ASSERT(m_flow);

                if (m_top_points)
                {
                        m_top_points->bind(POINTS_BINDING);
                }

                m_flow->bind(POINTS_FLOW_BINDING);

                if (m_flow_guess)
                {
                        m_flow_guess->bind(POINTS_FLOW_GUESS_BINDING);
                }

                m_buffer.bind(DATA_BINDING);
        }
};

class FlowImagesMemory final
{
        static constexpr int IMAGES_BINDING = 4;

        opengl::UniformBuffer m_buffer;

        struct Images
        {
                GLuint64 image_dx;
                alignas(16) GLuint64 image_dy;
                alignas(16) GLuint64 image_i;
                alignas(16) GLuint64 texture_j;
        };

public:
        FlowImagesMemory(const ImageR32F& image_dx, const ImageR32F& image_dy, const ImageR32F& image_i,
                         const ImageR32F& texture_j)
                : m_buffer(sizeof(Images))
        {
                Images images;

                images.image_dx = image_dx.image_read_handle();
                images.image_dy = image_dy.image_read_handle();
                images.image_i = image_i.image_read_handle();
                images.texture_j = texture_j.texture_handle();

                m_buffer.copy(images);
        }

        void bind() const
        {
                m_buffer.bind(IMAGES_BINDING);
        }
};

std::array<GrayscaleMemory, 2> create_grayscale_memory(const opengl::TextureRGBA32F& source_image, const Pyramid& pyramid)
{
        return {GrayscaleMemory(source_image, pyramid.images(0)[0]), GrayscaleMemory(source_image, pyramid.images(1)[0])};
}

vec2i create_grayscale_groups(const Pyramid& pyramid)
{
        int x = group_count(pyramid.width(0), GROUP_SIZE);
        int y = group_count(pyramid.height(0), GROUP_SIZE);

        return {x, y};
}

std::array<std::vector<DownsampleMemory>, 2> create_downsample_memory(const Pyramid& pyramid)
{
        std::array<std::vector<DownsampleMemory>, 2> downsample_images;

        for (unsigned i = 1; i < pyramid.size(); ++i)
        {
                ASSERT(pyramid.width(i - 1) > pyramid.width(i) || pyramid.height(i - 1) > pyramid.height(i));

                downsample_images[0].emplace_back(pyramid.images(0)[i - 1], pyramid.images(0)[i]);
                downsample_images[1].emplace_back(pyramid.images(1)[i - 1], pyramid.images(1)[i]);
        }

        return downsample_images;
}

std::vector<vec2i> create_downsample_groups(const Pyramid& pyramid)
{
        std::vector<vec2i> groups;

        for (unsigned i = 1; i < pyramid.size(); ++i)
        {
                int x = group_count(pyramid.width(i), GROUP_SIZE);
                int y = group_count(pyramid.height(i), GROUP_SIZE);
                groups.push_back({x, y});
        }

        return groups;
}

std::array<std::vector<SobelMemory>, 2> create_sobel_memory(const Pyramid& pyramid)
{
        std::array<std::vector<SobelMemory>, 2> sobel_images;

        for (size_t i = 0; i < pyramid.size(); ++i)
        {
                sobel_images[0].emplace_back(pyramid.images(0)[i], pyramid.dx()[i], pyramid.dy()[i]);
                sobel_images[1].emplace_back(pyramid.images(1)[i], pyramid.dx()[i], pyramid.dy()[i]);
        }

        return sobel_images;
}

std::vector<vec2i> create_sobel_groups(const Pyramid& pyramid)
{
        std::vector<vec2i> groups;

        for (size_t i = 0; i < pyramid.size(); ++i)
        {
                int x = group_count(pyramid.width(i), GROUP_SIZE);
                int y = group_count(pyramid.height(i), GROUP_SIZE);
                groups.push_back({x, y});
        }

        return groups;
}

std::vector<FlowDataMemory> create_flow_data_memory(const Pyramid& pyramid, int top_x, int top_y,
                                                    const opengl::StorageBuffer& top_points,
                                                    const opengl::StorageBuffer& top_flow)
{
        FlowDataMemory::Data data;

        std::vector<FlowDataMemory> flow_data(pyramid.size());

        for (size_t i = 0; i < pyramid.size(); ++i)
        {
                const bool top = (i == 0);
                const bool bottom = (i + 1 == pyramid.size());

                if (!top)
                {
                        // Не самый верхний уровень, поэтому расчёт для всех точек
                        flow_data[i].set_top_points(nullptr);
                        flow_data[i].set_flow(&pyramid.flow()[i]);
                        data.use_all_points = 1;
                        data.point_count_x = pyramid.width(i);
                        data.point_count_y = pyramid.height(i);
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
                        data.guess_kx = (pyramid.width(i_prev) != pyramid.width(i)) ? 2 : 1;
                        data.guess_ky = (pyramid.height(i_prev) != pyramid.height(i)) ? 2 : 1;
                        data.guess_width = pyramid.width(i_prev);
                        flow_data[i].set_flow_guess(&pyramid.flow()[i_prev]);
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

std::array<std::vector<FlowImagesMemory>, 2> create_flow_images_memory(const Pyramid& pyramid)
{
        std::array<std::vector<FlowImagesMemory>, 2> flow_images;

        for (size_t i = 0; i < pyramid.size(); ++i)
        {
                flow_images[0].emplace_back(pyramid.dx()[i], pyramid.dy()[i], pyramid.images(0)[i], pyramid.images(1)[i]);
                flow_images[1].emplace_back(pyramid.dx()[i], pyramid.dy()[i], pyramid.images(1)[i], pyramid.images(0)[i]);
        }

        return flow_images;
}

std::vector<vec2i> create_flow_groups(const std::vector<FlowDataMemory>& flow_data)
{
        std::vector<vec2i> groups;

        for (size_t i = 0; i < flow_data.size(); ++i)
        {
                int x = group_count(flow_data[i].point_count_x(), GROUP_SIZE);
                int y = group_count(flow_data[i].point_count_y(), GROUP_SIZE);
                groups.push_back({x, y});
        }

        return groups;
}

class Impl final : public gpgpu_opengl::OpticalFlowCompute
{
        Pyramid m_pyramid;

        std::array<GrayscaleMemory, 2> m_grayscale_memory;
        vec2i m_grayscale_groups;
        opengl::ComputeProgram m_grayscale_compute;

        std::array<std::vector<DownsampleMemory>, 2> m_downsample_memory;
        std::vector<vec2i> m_downsample_groups;
        opengl::ComputeProgram m_downsample_compute;

        std::array<std::vector<SobelMemory>, 2> m_sobel_memory;
        std::vector<vec2i> m_sobel_groups;
        opengl::ComputeProgram m_sobel_compute;

        std::vector<FlowDataMemory> m_flow_data_memory;
        std::array<std::vector<FlowImagesMemory>, 2> m_flow_images_memory;
        std::vector<vec2i> m_flow_groups;
        opengl::ComputeProgram m_flow_compute;

        int m_i_index = -1;

        void build_image_pyramid(int index) const
        {
                ASSERT(index == 0 || index == 1);
                ASSERT(m_downsample_memory[index].size() + 1 == m_pyramid.size());
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
                return m_pyramid.dx()[0].texture_handle();
        }
        GLuint64 image_pyramid_texture() const override
        {
                return m_pyramid.images(m_i_index)[0].texture_handle();
        }

public:
        Impl(int width, int height, const opengl::TextureRGBA32F& source_image, int top_x, int top_y,
             const opengl::StorageBuffer& top_points, const opengl::StorageBuffer& top_flow)
                : m_pyramid(pyramid_sizes(width, height, BOTTOM_IMAGE_SIZE)),
                  //
                  m_grayscale_memory(create_grayscale_memory(source_image, m_pyramid)),
                  m_grayscale_groups(create_grayscale_groups(m_pyramid)),
                  m_grayscale_compute(opengl::ComputeShader(grayscale_source())),
                  //
                  m_downsample_memory(create_downsample_memory(m_pyramid)),
                  m_downsample_groups(create_downsample_groups(m_pyramid)),
                  m_downsample_compute(opengl::ComputeShader(downsample_source())),
                  //
                  m_sobel_memory(create_sobel_memory(m_pyramid)),
                  m_sobel_groups(create_sobel_groups(m_pyramid)),
                  m_sobel_compute(opengl::ComputeShader(sobel_source())),
                  //
                  m_flow_data_memory(create_flow_data_memory(m_pyramid, top_x, top_y, top_points, top_flow)),
                  m_flow_images_memory(create_flow_images_memory(m_pyramid)),
                  m_flow_groups(create_flow_groups(m_flow_data_memory)),
                  m_flow_compute(opengl::ComputeShader(flow_source()))
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
