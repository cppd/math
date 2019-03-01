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

class FlowMemory final
{
        static constexpr int POINTS_BINDING = 0;
        static constexpr int POINTS_FLOW_BINDING = 1;
        static constexpr int POINTS_FLOW_GUESS_BINDING = 2;
        static constexpr int DATA_BINDING = 3;

        const opengl::StorageBuffer* m_top_points = nullptr;
        const opengl::StorageBuffer* m_points_flow = nullptr;
        const opengl::StorageBuffer* m_points_flow_guess = nullptr;

        opengl::UniformBuffer m_buffer;

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

public:
        FlowMemory() : m_buffer(sizeof(Data))
        {
        }

        void set_use_all_points()
        {
                m_top_points = nullptr;

                decltype(Data().use_all_points) use_all_points = 1;
                m_buffer.copy(offsetof(Data, use_all_points), use_all_points);
        }

        void set_use_top_points(const opengl::StorageBuffer& top_points)
        {
                m_top_points = &top_points;

                decltype(Data().use_all_points) use_all_points = 0;
                m_buffer.copy(offsetof(Data, use_all_points), use_all_points);
        }

        void set_point_count(int x, int y) const
        {
                constexpr size_t size = sizeof(Data::point_count_x) + sizeof(Data::point_count_y);
                constexpr size_t offset = offsetof(Data, point_count_x);

                static_assert(offsetof(Data, use_all_points) == offset + size);

                Data data;
                data.point_count_x = x;
                data.point_count_y = y;

                m_buffer.copy(offset, offset, size, data);
        }

        void set_no_guess()
        {
                m_points_flow_guess = nullptr;

                decltype(Data().use_guess) d = 0;
                m_buffer.copy(offsetof(Data, use_guess), d);
        }

        void set_guess(const opengl::StorageBuffer& points_flow_guess, int guess_kx, int guess_ky, int guess_width)
        {
                m_points_flow_guess = &points_flow_guess;

                constexpr size_t size =
                        sizeof(Data::use_guess) + sizeof(Data::guess_kx) + sizeof(Data::guess_ky) + sizeof(Data::guess_width);
                constexpr size_t offset = offsetof(Data, use_guess);

                static_assert(sizeof(Data) == offset + size);

                Data data;
                data.use_guess = 1;
                data.guess_kx = guess_kx;
                data.guess_ky = guess_ky;
                data.guess_width = guess_width;

                m_buffer.copy(offset, offset, size, data);
        }

        void set_points_flow(const opengl::StorageBuffer& buffer)
        {
                m_points_flow = &buffer;
        }

        void bind() const
        {
                ASSERT(m_points_flow);

                if (m_top_points)
                {
                        m_top_points->bind(POINTS_BINDING);
                }

                m_points_flow->bind(POINTS_FLOW_BINDING);

                if (m_points_flow_guess)
                {
                        m_points_flow_guess->bind(POINTS_FLOW_GUESS_BINDING);
                }

                m_buffer.bind(DATA_BINDING);
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

class Impl final : public gpgpu_opengl::OpticalFlowCompute
{
        static constexpr int IMAGE_DX_LOCATION = 0;
        static constexpr int IMAGE_DY_LOCATION = 1;
        static constexpr int IMAGE_I_LOCATION = 2;
        static constexpr int TEXTURE_J_LOCATION = 3;

        int m_top_x;
        int m_top_y;

        const opengl::StorageBuffer& m_top_points;
        const opengl::StorageBuffer& m_top_flow;

        ProgramGrayscale m_program_grayscale;
        ProgramDownsample m_program_downsample;
        ProgramSobel m_program_sobel;
        opengl::ComputeProgram m_comp_flow;

        FlowMemory m_flow_memory;

        ImagePyramid m_image_pyramid;

        int m_i_index = 0;
        int m_j_index = 1;
        bool m_image_i_exists = false;

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

        void compute_optical_flow(const ImagePyramid& pyramid, int i_index, int j_index)
        {
                ASSERT((i_index == 0 || i_index == 1) && (i_index == 1 - j_index));

                int size = pyramid.size();

                for (int i = size - 1; i >= 0; --i)
                {
                        if (i != 0)
                        {
                                // Если не самый верхний уровень, то расчёт для всех точек
                                m_flow_memory.set_use_all_points();
                                m_flow_memory.set_points_flow(pyramid.flow()[i]);
                                m_flow_memory.set_point_count(pyramid.width(i), pyramid.height(i));
                        }
                        else
                        {
                                // Если самый верхний уровень, то расчёт только для заданных точек для рисования на экране
                                m_flow_memory.set_use_top_points(m_top_points);
                                m_flow_memory.set_points_flow(m_top_flow);
                                m_flow_memory.set_point_count(m_top_x, m_top_y);
                        }

                        if (i != size - 1)
                        {
                                // Если не самый нижний уровень, то в качестве приближения использовать поток,
                                // полученный на меньших изображениях
                                int i_prev = i + 1;
                                int guess_kx = (pyramid.width(i_prev) != pyramid.width(i)) ? 2 : 1;
                                int guess_ky = (pyramid.height(i_prev) != pyramid.height(i)) ? 2 : 1;
                                int guess_width = pyramid.width(i_prev);
                                m_flow_memory.set_guess(pyramid.flow()[i_prev], guess_kx, guess_ky, guess_width);
                        }
                        else
                        {
                                // Самый нижний уровень пирамиды, поэтому нет начального потока
                                m_flow_memory.set_no_guess();
                        }

                        m_comp_flow.set_uniform_handle(IMAGE_DX_LOCATION, pyramid.dx()[i].image_read_handle());
                        m_comp_flow.set_uniform_handle(IMAGE_DY_LOCATION, pyramid.dy()[i].image_read_handle());
                        m_comp_flow.set_uniform_handle(IMAGE_I_LOCATION, pyramid.images(i_index)[i].image_read_handle());
                        m_comp_flow.set_uniform_handle(TEXTURE_J_LOCATION, pyramid.images(j_index)[i].texture_handle());

                        m_flow_memory.bind();

                        int groups_x = group_count(i != 0 ? pyramid.width(i) : m_top_x, GROUP_SIZE);
                        int groups_y = group_count(i != 0 ? pyramid.height(i) : m_top_y, GROUP_SIZE);
                        m_comp_flow.dispatch_compute(groups_x, groups_y, 1);

                        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
                }
        }

        void reset() override
        {
                m_image_i_exists = false;
        }

        bool exec() override
        {
                // Обозначения: i - предыдущее изображение, j - следующее изображение

                std::swap(m_i_index, m_j_index);

                build_image_pyramid(m_image_pyramid.images(m_j_index));

                if (!m_image_i_exists)
                {
                        m_image_i_exists = true;
                        return false;
                }

                compute_dxdy(m_image_pyramid.images(m_i_index), m_image_pyramid.dx(), m_image_pyramid.dy());

                compute_optical_flow(m_image_pyramid, m_i_index, m_j_index);

                return true;
        }

        GLuint64 image_pyramid_dx_texture() const override
        {
                return m_image_pyramid.dx()[0].texture_handle();
        }
        GLuint64 image_pyramid_texture() const override
        {
                return m_image_pyramid.images(m_i_index)[0].texture_handle();
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
                  m_image_pyramid(width, height)
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
