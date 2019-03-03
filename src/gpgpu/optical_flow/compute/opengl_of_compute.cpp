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

std::string downsample_source()
{
        std::string s;
        s += "const uint GROUP_SIZE = " + to_string(GROUP_SIZE) + ";\n";
        return s + downsample_shader;
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

std::string grayscale_source()
{
        std::string s;
        s += "const uint GROUP_SIZE = " + to_string(GROUP_SIZE) + ";\n";
        return s + grayscale_shader;
}

std::string sobel_source()
{
        std::string s;
        s += "const uint GROUP_SIZE = " + to_string(GROUP_SIZE) + ";\n";
        return s + sobel_shader;
}

class FlowData final
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

        FlowData() : m_buffer(sizeof(Data))
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

class FlowImages final
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
        FlowImages(GLuint64 image_dx, GLuint64 image_dy, GLuint64 image_i, GLuint64 texture_j) : m_buffer(sizeof(Images))
        {
                Images images;

                images.image_dx = image_dx;
                images.image_dy = image_dy;
                images.image_i = image_i;
                images.texture_j = texture_j;

                m_buffer.copy(images);
        }

        void bind() const
        {
                m_buffer.bind(IMAGES_BINDING);
        }
};

class ProgramGrayscale final
{
        static constexpr int SRC_LOCATION = 0;
        static constexpr int DST_LOCATION = 1;

        int m_groups_x;
        int m_groups_y;
        opengl::ComputeProgram m_program;

public:
        ProgramGrayscale(const opengl::TextureRGBA32F& source_image)
                : m_groups_x(group_count(source_image.texture().width(), GROUP_SIZE)),
                  m_groups_y(group_count(source_image.texture().height(), GROUP_SIZE)),
                  m_program(opengl::ComputeShader(grayscale_source()))
        {
                m_program.set_uniform_handle(SRC_LOCATION, source_image.image_resident_handle_read_only());
        }

        void exec(const ImageR32F& image) const
        {
                m_program.set_uniform_handle(DST_LOCATION, image.image_write_handle());
                m_program.dispatch_compute(m_groups_x, m_groups_y, 1);
        }
};

class ProgramDownsample final
{
        static constexpr int BIG_LOCATION = 0;
        static constexpr int SMALL_LOCATION = 1;

        opengl::ComputeProgram m_program;

public:
        ProgramDownsample() : m_program(opengl::ComputeShader(downsample_source()))
        {
        }

        void exec(const ImageR32F& big, const ImageR32F& small) const
        {
                m_program.set_uniform_handle(BIG_LOCATION, big.image_read_handle());
                m_program.set_uniform_handle(SMALL_LOCATION, small.image_write_handle());

                int groups_x = group_count(small.width(), GROUP_SIZE);
                int groups_y = group_count(small.height(), GROUP_SIZE);
                m_program.dispatch_compute(groups_x, groups_y, 1);
        }
};

class ProgramSobel final
{
        static constexpr int IMAGE_LOCATION = 0;
        static constexpr int DX_LOCATION = 1;
        static constexpr int DY_LOCATION = 2;

        opengl::ComputeProgram m_program;

public:
        ProgramSobel() : m_program(opengl::ComputeShader(sobel_source()))
        {
        }

        void exec(const ImageR32F& image, const ImageR32F& dx, const ImageR32F& dy) const
        {
                m_program.set_uniform_handle(IMAGE_LOCATION, image.image_read_handle());
                m_program.set_uniform_handle(DX_LOCATION, dx.image_write_handle());
                m_program.set_uniform_handle(DY_LOCATION, dy.image_write_handle());

                int groups_x = group_count(image.width(), GROUP_SIZE);
                int groups_y = group_count(image.height(), GROUP_SIZE);
                m_program.dispatch_compute(groups_x, groups_y, 1);
        }
};

std::vector<vec2i> create_image_pyramid_sizes(int width, int height, int min_size)
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

void create_textures(const std::vector<vec2i>& sizes, std::vector<ImageR32F>* textures)
{
        textures->clear();
        textures->reserve(sizes.size());
        for (const vec2i& s : sizes)
        {
                textures->emplace_back(s[0], s[1]);
        }
}

void create_buffers(const std::vector<vec2i>& sizes, std::vector<opengl::StorageBuffer>* buffers)
{
        buffers->clear();
        buffers->reserve(sizes.size());
        for (const vec2i& s : sizes)
        {
                buffers->emplace_back(s[0] * s[1] * sizeof(vec2f));
        }
}

class ImagePyramid final
{
        std::array<std::vector<ImageR32F>, 2> m_images;
        std::vector<ImageR32F> m_dx;
        std::vector<ImageR32F> m_dy;
        std::vector<opengl::StorageBuffer> m_flow;

public:
        ImagePyramid(int width, int height)
        {
                std::vector<vec2i> sizes = create_image_pyramid_sizes(width, height, BOTTOM_IMAGE_SIZE);
                create_textures(sizes, &m_images[0]);
                create_textures(sizes, &m_images[1]);
                create_textures(sizes, &m_dx);
                create_textures(sizes, &m_dy);
                create_buffers(sizes, &m_flow);
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

std::vector<FlowData> create_flow_data(const ImagePyramid& pyramid, int top_x, int top_y, const opengl::StorageBuffer& top_points,
                                       const opengl::StorageBuffer& top_flow)
{
        FlowData::Data data;

        std::vector<FlowData> flow_data(pyramid.size());

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

std::array<std::vector<FlowImages>, 2> create_flow_images(const ImagePyramid& pyramid)
{
        std::array<std::vector<FlowImages>, 2> flow_images;

        for (size_t i = 0; i < pyramid.size(); ++i)
        {
                flow_images[0].emplace_back(pyramid.dx()[i].image_read_handle(), pyramid.dy()[i].image_read_handle(),
                                            pyramid.images(0)[i].image_read_handle(), pyramid.images(1)[i].texture_handle());

                flow_images[1].emplace_back(pyramid.dx()[i].image_read_handle(), pyramid.dy()[i].image_read_handle(),
                                            pyramid.images(1)[i].image_read_handle(), pyramid.images(0)[i].texture_handle());
        }

        return flow_images;
}

class Impl final : public gpgpu_opengl::OpticalFlowCompute
{
        int m_top_x;
        int m_top_y;

        const opengl::StorageBuffer& m_top_points;
        const opengl::StorageBuffer& m_top_flow;

        ProgramGrayscale m_program_grayscale;
        ProgramDownsample m_program_downsample;
        ProgramSobel m_program_sobel;
        opengl::ComputeProgram m_comp_flow;

        ImagePyramid m_pyramid;

        std::vector<FlowData> m_flow_data;
        std::array<std::vector<FlowImages>, 2> m_flow_images;

        int m_i_index = -1;

        void build_image_pyramid(const std::vector<ImageR32F>& images) const
        {
                // Уровень 0 заполняется по исходному изображению
                m_program_grayscale.exec(images[0]);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

                // Каждый следующий уровень меньше предыдущего
                for (unsigned i = 1; i < images.size(); ++i)
                {
                        ASSERT(images[i - 1].width() > images[i].width() || images[i - 1].height() > images[i].height());

                        m_program_downsample.exec(images[i - 1], images[i]);
                        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
                }
        }

        void compute_dxdy(const std::vector<ImageR32F>& images, const std::vector<ImageR32F>& images_dx,
                          const std::vector<ImageR32F>& images_dy) const
        {
                ASSERT(images.size() == images_dx.size());
                ASSERT(images.size() == images_dy.size());

                for (unsigned i = 0; i < images.size(); ++i)
                {
                        m_program_sobel.exec(images[i], images_dx[i], images_dy[i]);
                }

                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }

        void compute_optical_flow(int i_index)
        {
                ASSERT(i_index == 0 || i_index == 1);
                ASSERT(m_flow_data.size() == m_flow_images[0].size());
                ASSERT(m_flow_data.size() == m_flow_images[1].size());

                int size = m_flow_data.size();

                for (int i = size - 1; i >= 0; --i)
                {
                        m_flow_data[i].bind();
                        m_flow_images[i_index][i].bind();

                        int groups_x = group_count(m_flow_data[i].point_count_x(), GROUP_SIZE);
                        int groups_y = group_count(m_flow_data[i].point_count_y(), GROUP_SIZE);
                        m_comp_flow.dispatch_compute(groups_x, groups_y, 1);

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
                        build_image_pyramid(m_pyramid.images(m_i_index));
                }
                else
                {
                        m_i_index = 1 - m_i_index;
                }

                // i — предыдущее изображение, 1-i — текущее изображение

                build_image_pyramid(m_pyramid.images(1 - m_i_index));

                compute_dxdy(m_pyramid.images(m_i_index), m_pyramid.dx(), m_pyramid.dy());

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
                : m_top_x(top_x),
                  m_top_y(top_y),
                  m_top_points(top_points),
                  m_top_flow(top_flow),
                  m_program_grayscale(source_image),
                  m_comp_flow(opengl::ComputeShader(flow_source())),
                  m_pyramid(width, height),
                  m_flow_data(create_flow_data(m_pyramid, m_top_x, m_top_y, m_top_points, m_top_flow)),
                  m_flow_images(create_flow_images(m_pyramid))
        {
        }

        Impl(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) = delete;
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
