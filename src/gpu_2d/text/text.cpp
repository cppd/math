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
#include "com/font/chars.h"
#include "com/font/font.h"
#include "com/font/vertices.h"
#include "graphics/opengl/objects.h"
#include "graphics/opengl/query.h"

#include <array>
#include <limits>
#include <thread>
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

static_assert(sizeof(TextVertex) == sizeof(Vector<2, GLint>) + sizeof(Vector<2, GLfloat>));
static_assert(std::is_same_v<decltype(TextVertex::v), Vector<2, GLint>>);
static_assert(std::is_same_v<decltype(TextVertex::t), Vector<2, GLfloat>>);

class Text::Impl final
{
        const std::thread::id m_thread_id;

        int m_step_y;
        int m_start_x;
        int m_start_y;

        opengl::VertexArray m_vertex_array;
        opengl::ArrayBuffer m_vertex_buffer;
        opengl::GraphicsProgram m_program;
        std::unordered_map<char, FontChar> m_chars;
        std::unique_ptr<opengl::TextureR32F> m_texture;

        template <typename T>
        void draw(int x, int y, const T& text) const
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                thread_local std::vector<TextVertex> vertices;

                text_vertices(m_chars, m_step_y, x, y, text, &vertices);

                opengl::GLEnableAndRestore<GL_BLEND> e;
                m_vertex_array.bind();
                m_vertex_buffer.load_dynamic_draw(vertices);
                m_program.draw_arrays(GL_TRIANGLES, 0, vertices.size());
        }

public:
        Impl(int size, int step_y, int start_x, int start_y, const Color& color, const mat4& matrix)
                : m_thread_id(std::this_thread::get_id()),
                  m_step_y(step_y),
                  m_start_x(start_x),
                  m_start_y(start_y),
                  m_program(opengl::VertexShader(text_vertex_shader), opengl::FragmentShader(text_fragment_shader))
        {
                m_vertex_array.attrib_i_pointer(0, 2, GL_INT, m_vertex_buffer, offsetof(TextVertex, v), sizeof(TextVertex), true);
                m_vertex_array.attrib_pointer(1, 2, GL_FLOAT, m_vertex_buffer, offsetof(TextVertex, t), sizeof(TextVertex), true);

                set_color(color);
                set_matrix(matrix);

                //

                int max_size = opengl::max_texture_size();

                Font font(size);
                int width, height;
                std::vector<std::uint_least8_t> pixels;
                create_font_chars(font, max_size, max_size, &m_chars, &width, &height, &pixels);

                m_texture = std::make_unique<opengl::TextureR32F>(width, height, pixels);
                m_program.set_uniform_handle("tex", m_texture->texture().texture_resident_handle());
        }

        ~Impl()
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);
        }

        void set_color(const Color& color) const
        {
                m_program.set_uniform("text_color", color.to_rgb_vector<float>());
        }

        void set_matrix(const mat4& matrix) const
        {
                m_program.set_uniform_float("matrix", matrix);
        }

        void draw(const std::vector<std::string>& text) const
        {
                draw(m_start_x, m_start_y, text);
        }

        void draw(const std::string& text) const
        {
                draw(m_start_x, m_start_y, text);
        }
};

Text::Text(int size, int step_y, int start_x, int start_y, const Color& color, const mat4& matrix)
        : m_impl(std::make_unique<Impl>(size, step_y, start_x, start_y, color, matrix))
{
}

Text::~Text() = default;

void Text::set_color(const Color& color) const
{
        m_impl->set_color(color);
}

void Text::set_matrix(const mat4& matrix) const
{
        m_impl->set_matrix(matrix);
}

void Text::draw(const std::vector<std::string>& text) const
{
        m_impl->draw(text);
}

void Text::draw(const std::string& text) const
{
        m_impl->draw(text);
}
