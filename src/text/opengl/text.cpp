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

#include "text.h"

#include "com/container.h"
#include "com/error.h"
#include "com/font/font.h"
#include "com/font/glyphs.h"
#include "com/font/vertices.h"
#include "graphics/opengl/buffers.h"
#include "graphics/opengl/capabilities.h"
#include "graphics/opengl/query.h"
#include "graphics/opengl/shader.h"

#include <array>
#include <optional>
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

constexpr int MATRICES_BINDING = 0;
constexpr int DRAWING_BINDING = 2;

namespace
{
class ShaderMemory
{
        struct Matrices
        {
                Matrix<4, 4, float> matrix;
        };

        struct Drawing
        {
                vec3f text_color;
        };

        opengl::UniformBuffer m_matrices;
        opengl::UniformBuffer m_drawing;

public:
        ShaderMemory() : m_matrices(sizeof(Matrices)), m_drawing(sizeof(Drawing))
        {
        }

        void set_matrix(const mat4& matrix) const
        {
                decltype(Matrices().matrix) m = transpose(to_matrix<float>(matrix));
                m_matrices.copy(offsetof(Matrices, matrix), m);
        }

        void set_color(const Color& color) const
        {
                decltype(Drawing().text_color) c = color.to_rgb_vector<float>();
                m_drawing.copy(offsetof(Drawing, text_color), c);
        }

        void bind_matrices(int point) const
        {
                m_matrices.bind(point);
        }

        void bind_drawing(int point) const
        {
                m_drawing.bind(point);
        }
};
}

class OpenGLText::Impl final
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        opengl::VertexArray m_vertex_array;
        mutable std::optional<opengl::ArrayBuffer> m_vertex_buffer;
        opengl::GraphicsProgram m_program;
        std::unordered_map<char32_t, FontGlyph> m_glyphs;
        std::unique_ptr<opengl::TextureR32F> m_texture;
        ShaderMemory m_shader_memory;

        template <typename T>
        void draw_text(int step_y, int x, int y, const T& text) const
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                thread_local std::vector<TextVertex> vertices;

                text_vertices(m_glyphs, step_y, x, y, text, &vertices);

                const size_t data_size = storage_size(vertices);

                if (!m_vertex_buffer || m_vertex_buffer->size() < data_size)
                {
                        m_vertex_buffer.emplace(data_size);
                        m_vertex_array.attrib_i(0, 2, GL_INT, *m_vertex_buffer, offsetof(TextVertex, v), sizeof(TextVertex));
                        m_vertex_array.attrib(1, 2, GL_FLOAT, *m_vertex_buffer, offsetof(TextVertex, t), sizeof(TextVertex));
                }

                m_vertex_buffer->write(vertices);

                opengl::GLEnableAndRestore<GL_BLEND> e;

                m_shader_memory.bind_matrices(MATRICES_BINDING);
                m_shader_memory.bind_drawing(DRAWING_BINDING);
                m_vertex_array.bind();
                m_program.draw_arrays(GL_TRIANGLES, 0, vertices.size());
        }

public:
        Impl(int size, const Color& color, const mat4& matrix)
                : m_program(opengl::VertexShader(text_vertex_shader), opengl::FragmentShader(text_fragment_shader))
        {
                set_color(color);
                set_matrix(matrix);

                //

                int max_size = opengl::max_texture_size();

                Font font(size);
                int width, height;
                std::vector<std::uint_least8_t> pixels;
                create_font_glyphs(font, max_size, max_size, &m_glyphs, &width, &height, &pixels);

                m_texture = std::make_unique<opengl::TextureR32F>(width, height, pixels);
                m_program.set_uniform_handle("tex", m_texture->texture().texture_resident_handle());
        }

        ~Impl()
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);
        }

        void set_color(const Color& color) const
        {
                m_shader_memory.set_color(color);
        }

        void set_matrix(const mat4& matrix) const
        {
                m_shader_memory.set_matrix(matrix);
        }

        void draw(int step_y, int x, int y, const std::vector<std::string>& text) const
        {
                draw_text(step_y, x, y, text);
        }

        void draw(int step_y, int x, int y, const std::string& text) const
        {
                draw_text(step_y, x, y, text);
        }
};

OpenGLText::OpenGLText(int size, const Color& color, const mat4& matrix) : m_impl(std::make_unique<Impl>(size, color, matrix))
{
}

OpenGLText::~OpenGLText() = default;

void OpenGLText::set_color(const Color& color) const
{
        m_impl->set_color(color);
}

void OpenGLText::set_matrix(const mat4& matrix) const
{
        m_impl->set_matrix(matrix);
}

void OpenGLText::draw(int step_y, int x, int y, const std::vector<std::string>& text) const
{
        m_impl->draw(step_y, x, y, text);
}

void OpenGLText::draw(int step_y, int x, int y, const std::string& text) const
{
        m_impl->draw(step_y, x, y, text);
}
