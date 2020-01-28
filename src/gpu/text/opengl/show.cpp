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

#if defined(OPENGL_FOUND)

#include "show.h"

#include "memory.h"
#include "shader_source.h"

#include "com/container.h"
#include "com/error.h"
#include "com/font/font.h"
#include "com/font/glyphs.h"
#include "com/font/vertices.h"
#include "com/matrix_alg.h"
#include "graphics/opengl/buffers.h"
#include "graphics/opengl/capabilities.h"
#include "graphics/opengl/query.h"
#include "graphics/opengl/shader.h"

#include <array>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

static_assert(sizeof(TextVertex) == sizeof(Vector<2, GLint>) + sizeof(Vector<2, GLfloat>));
static_assert(std::is_same_v<decltype(TextVertex::v), Vector<2, GLint>>);
static_assert(std::is_same_v<decltype(TextVertex::t), Vector<2, GLfloat>>);

constexpr GLenum TEXTURE_FORMAT = GL_R32F;

namespace gpu_opengl
{
namespace
{
class Impl final : public Text
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        opengl::VertexArray m_vertex_array;
        std::optional<opengl::Buffer> m_vertex_buffer;
        opengl::GraphicsProgram m_program;
        std::unordered_map<char32_t, FontGlyph> m_glyphs;
        std::unique_ptr<opengl::Texture> m_texture;
        TextShaderMemory m_shader_memory;

        int m_x = -1, m_y = -1, m_width = -1, m_height = -1;

        void set_color(const Color& color) const override
        {
                m_shader_memory.set_color(color);
        }

        void set_window(int x, int y, int width, int height) override
        {
                m_x = x;
                m_y = y;
                m_width = width;
                m_height = height;

                // Матрица для рисования на плоскости окна, точка (0, 0) слева вверху
                double left = 0;
                double right = m_width;
                double bottom = m_height;
                double top = 0;
                double near = 1;
                double far = -1;
                m_shader_memory.set_matrix(ortho_opengl<double>(left, right, bottom, top, near, far));
        }

        void draw(const TextData& text_data) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                ASSERT(m_x >= 0 && m_y >= 0 && m_width > 0 && m_height > 0);

                thread_local std::vector<TextVertex> vertices;

                text_vertices(m_glyphs, text_data, &vertices);

                const size_t data_size = storage_size(vertices);

                if (!m_vertex_buffer || m_vertex_buffer->size() < data_size)
                {
                        m_vertex_buffer.emplace(data_size, GL_MAP_WRITE_BIT);
                        m_vertex_array.attrib_i(
                                0, 2, GL_INT, *m_vertex_buffer, offsetof(TextVertex, v), sizeof(TextVertex));
                        m_vertex_array.attrib(
                                1, 2, GL_FLOAT, *m_vertex_buffer, offsetof(TextVertex, t), sizeof(TextVertex));
                }

                opengl::BufferMapper(*m_vertex_buffer, 0, data_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)
                        .write(vertices);

                opengl::GLEnableAndRestore<GL_BLEND> e;

                glViewport(m_x, m_y, m_width, m_height);
                m_shader_memory.bind();
                m_vertex_array.bind();
                m_program.draw_arrays(GL_TRIANGLES, 0, vertices.size());
        }

public:
        Impl(int size, const Color& color)
                : m_program(opengl::VertexShader(text_vert()), opengl::FragmentShader(text_frag()))
        {
                set_color(color);

                //

                int max_size = opengl::max_texture_size();

                Font font(size);
                int width, height;
                std::vector<std::uint_least8_t> pixels;
                create_font_glyphs(font, max_size, max_size, &m_glyphs, &width, &height, &pixels);

                m_texture = std::make_unique<opengl::Texture>(TEXTURE_FORMAT, width, height, pixels);
                m_program.set_uniform_handle("tex", m_texture->texture_handle());
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);
        }
};
}

std::unique_ptr<Text> create_text(int size, const Color& color)
{
        return std::make_unique<Impl>(size, color);
}
}

#endif
