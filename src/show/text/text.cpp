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

#include "text.h"

#include "com/error.h"
#include "com/font.h"
#include "graphics/opengl/objects.h"

#include <array>
#include <limits>
#include <unordered_map>

// clang-format off
constexpr const char text_vertex_shader[]
{
#include "text.vert.str"
};
constexpr const char text_fragment_shader[]
{
#include "text.frag.str"
};
// clang-format on

namespace
{
struct Vertex final
{
        float v1, v2; // Координаты вершины в пространстве.
        float t1, t2; // Координаты вершины в текстуре.
        Vertex()
        {
        }
        Vertex(float v1_, float v2_, float t1_, float t2_) : v1(v1_), v2(v2_), t1(t1_), t2(t2_)
        {
        }
};

std::vector<float> integer_pixels_to_float_pixels(long long width, long long height, const unsigned char* pixels)
{
        static_assert(std::numeric_limits<unsigned char>::digits == 8);

        std::vector<float> buffer(width * height);
        for (size_t i = 0; i < buffer.size(); ++i)
        {
                buffer[i] = pixels[i] / 255.0f;
        }
        return buffer;
}

class CharData final
{
        const opengl::TextureR32F texture;

public:
        const int width, height, left, top, advance;
        const GLuint64 texture_handle;

        CharData(const Font::Char& c)
                : texture(c.width, c.height, integer_pixels_to_float_pixels(c.width, c.height, c.image)),
                  width(c.width),
                  height(c.height),
                  left(c.left),
                  top(c.top),
                  advance(c.advance_x),
                  texture_handle(texture.texture().texture_resident_handle())
        {
        }
};
}

class Text::Impl final
{
        Font m_font;

        int m_step_y;
        int m_start_x;
        int m_start_y;

        opengl::VertexArray m_vertex_array;
        opengl::ArrayBuffer m_vertex_buffer;
        opengl::GraphicsProgram m_program;

        std::unordered_map<char, CharData> m_chars;

        const CharData& char_data(char c)
        {
                auto iter = m_chars.find(c);
                if (iter == m_chars.end())
                {
                        iter = m_chars.try_emplace(c, m_font.render_char(c)).first;
                }

                return iter->second;
        }

public:
        Impl(int size, int step_y, int start_x, int start_y)
                : m_font(size),
                  m_step_y(step_y),
                  m_start_x(start_x),
                  m_start_y(start_y),
                  m_program(opengl::VertexShader(text_vertex_shader), opengl::FragmentShader(text_fragment_shader))
        {
                m_vertex_array.attrib_pointer(0, 3, GL_FLOAT, m_vertex_buffer, offsetof(Vertex, v1), sizeof(Vertex), true);
                m_vertex_array.attrib_pointer(1, 2, GL_FLOAT, m_vertex_buffer, offsetof(Vertex, t1), sizeof(Vertex), true);
        }

        void set_color(const Color& color)
        {
                m_program.set_uniform("text_color", color.to_rgb_vector<float>());
        }

        void draw(int width, int height, const std::vector<std::string>& text)
        {
                m_vertex_array.bind();

                float sx = 2.0f / width;
                float sy = 2.0f / height;

                float x = m_start_x;
                float y = m_start_y;

                for (const std::string& line : text)
                {
                        for (char c : line)
                        {
                                const CharData& cd = char_data(c);

                                m_program.set_uniform_handle("tex", cd.texture_handle);

                                float x2 = -1.0f + (x + cd.left) * sx;
                                float y2 = 1.0f - (y - cd.top) * sy;

                                std::array<Vertex, 4> vertices;

                                vertices[0] = Vertex(x2, y2, 0, 0);
                                vertices[1] = Vertex(x2 + cd.width * sx, y2, 1, 0);
                                vertices[2] = Vertex(x2, y2 - cd.height * sy, 0, 1);
                                vertices[3] = Vertex(x2 + cd.width * sx, y2 - cd.height * sy, 1, 1);

                                m_vertex_buffer.load_dynamic_draw(vertices);
                                m_program.draw_arrays(GL_TRIANGLE_STRIP, 0, vertices.size());

                                x += cd.advance;
                        }

                        y += m_step_y;
                        x = m_start_x;
                }
        }
};

Text::Text(int size, int step_y, int start_x, int start_y) : m_impl(std::make_unique<Impl>(size, step_y, start_x, start_y))
{
}

Text::~Text() = default;

void Text::set_color(const Color& color)
{
        m_impl->set_color(color);
}

void Text::draw(int width, int height, const std::vector<std::string>& text)
{
        m_impl->draw(width, height, text);
}
