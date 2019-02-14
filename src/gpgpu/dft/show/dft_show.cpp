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

#include "dft_show.h"

#include "com/math.h"
#include "gpgpu/dft/compute/dft_compute.h"
#include "graphics/opengl/buffers.h"
#include "graphics/opengl/shader.h"

#include <array>
#include <vector>

// clang-format off
constexpr const char vertex_shader[]
{
#include "dft_show.vert.str"
};
constexpr const char fragment_shader[]
{
#include "dft_show.frag.str"
};
// clang-format on

namespace
{
vec4f color_to_vec4f(const Color& c)
{
        return vec4f(c.red(), c.green(), c.blue(), 1);
}

struct Vertex
{
        static_assert(sizeof(Vector<4, float>) == 4 * sizeof(GLfloat));
        static_assert(sizeof(Vector<2, float>) == 2 * sizeof(GLfloat));

        Vector<4, GLfloat> v; // Конечные координаты вершины
        Vector<2, GLfloat> t; // Координаты вершины в текстуре (0 или 1)
};

class ShaderMemory
{
        static constexpr int DATA_BINDING = 0;

        opengl::UniformBuffer m_buffer;

        struct Data
        {
                Vector<4, GLfloat> background_color;
                Vector<4, GLfloat> foreground_color;
                GLfloat brightness;
        };

public:
        ShaderMemory() : m_buffer(sizeof(Data))
        {
        }

        void set_brightness(double brightness) const
        {
                decltype(Data().brightness) b = brightness;
                m_buffer.copy(offsetof(Data, brightness), b);
        }

        void set_background_color(const Color& color) const
        {
                decltype(Data().background_color) c = color_to_vec4f(color);
                m_buffer.copy(offsetof(Data, background_color), c);
        }

        void set_foreground_color(const Color& color) const
        {
                decltype(Data().foreground_color) c = color_to_vec4f(color);
                m_buffer.copy(offsetof(Data, foreground_color), c);
        }

        void bind() const
        {
                m_buffer.bind(DATA_BINDING);
        }
};

class Impl final : public gpgpu_opengl::DFTShow
{
        static constexpr int VERTEX_COUNT = 4;

        const bool m_source_srgb;
        opengl::TextureRGBA32F m_image_texture;
        std::unique_ptr<gpgpu_opengl::DFTComputeTexture> m_dft;
        opengl::VertexArray m_vertex_array;
        opengl::ArrayBuffer m_vertex_buffer;
        opengl::GraphicsProgram m_draw_prog;
        ShaderMemory m_shader_memory;

        void set_brightness(double brightness) override
        {
                m_shader_memory.set_brightness(brightness);
        }

        void set_background_color(const Color& color) override
        {
                m_shader_memory.set_background_color(color);
        }

        void set_color(const Color& color) override
        {
                m_shader_memory.set_foreground_color(color);
        }

        void take_image_from_framebuffer() override
        {
                m_image_texture.copy_texture_sub_image();
        }

        void draw() override
        {
                m_dft->exec(false, m_source_srgb);

                m_shader_memory.bind();
                m_vertex_array.bind();
                m_draw_prog.draw_arrays(GL_TRIANGLE_STRIP, 0, VERTEX_COUNT);
        }

public:
        Impl(int width, int height, int dst_x, int dst_y, const mat4& matrix, bool source_srgb, double brightness,
             const Color& background_color, const Color& color)
                : m_source_srgb(source_srgb),
                  m_image_texture(width, height),
                  m_dft(gpgpu_opengl::create_dft_compute_texture(width, height, m_image_texture)),
                  m_vertex_buffer(sizeof(Vertex) * VERTEX_COUNT),
                  m_draw_prog(opengl::VertexShader(vertex_shader), opengl::FragmentShader(fragment_shader))
        {
                m_vertex_array.attrib(0, 4, GL_FLOAT, m_vertex_buffer, offsetof(Vertex, v), sizeof(Vertex));
                m_vertex_array.attrib(1, 2, GL_FLOAT, m_vertex_buffer, offsetof(Vertex, t), sizeof(Vertex));

                m_draw_prog.set_uniform_handle("tex", m_image_texture.texture().texture_resident_handle());

                set_brightness(brightness);
                set_background_color(background_color);
                set_color(color);

                int x0 = dst_x;
                int y0 = dst_y;
                int x1 = x0 + width;
                int y1 = y0 + height;

                std::array<Vertex, VERTEX_COUNT> vertices;

                // Текстурный 0 находится внизу, поэтому текстурная
                // координата по Y для y0 равна 1, а для y1 равна 0
                vertices[0] = {to_vector<float>(matrix * vec4(x0, y0, 0, 1)), {0, 1}};
                vertices[1] = {to_vector<float>(matrix * vec4(x1, y0, 0, 1)), {1, 1}};
                vertices[2] = {to_vector<float>(matrix * vec4(x0, y1, 0, 1)), {0, 0}};
                vertices[3] = {to_vector<float>(matrix * vec4(x1, y1, 0, 1)), {1, 0}};

                m_vertex_buffer.write(vertices);
        }
};
}

namespace gpgpu_opengl
{
std::unique_ptr<DFTShow> create_dft_show(int width, int height, int dst_x, int dst_y, const mat4& matrix, bool source_srgb,
                                         double brightness, const Color& background_color, const Color& color)
{
        return std::make_unique<Impl>(width, height, dst_x, dst_y, matrix, source_srgb, brightness, background_color, color);
}
}
