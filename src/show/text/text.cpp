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
#include "graphics/objects.h"
#include "show/color_space/color_space.h"

#include <SFML/Graphics/Image.hpp>
#include <ft2build.h>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include FT_FREETYPE_H

constexpr int FONT_SIZE = 12;
constexpr int STEP_Y = 16;
constexpr int START_X = 10;
constexpr int START_Y = 20;

// clang-format off
constexpr const char text_vertex_shader[]
{
#include "text.vert.str"
};
constexpr const char text_fragment_shader[]
{
#include "text.frag.str"
};
constexpr const FT_Byte font_bytes[]
{
#include "DejaVuSans.ttf.bin"
};
// clang-format on

namespace
{
class Library final
{
        FT_Library m_ft;

public:
        Library(const Library&) = delete;
        Library& operator=(const Library&) = delete;
        Library(Library&&) = delete;
        Library& operator=(Library&&) = delete;

        Library()
        {
                if (FT_Init_FreeType(&m_ft))
                {
                        error("Error init freetype library");
                }
        }
        ~Library()
        {
                FT_Done_FreeType(m_ft);
        }
        FT_Library& get()
        {
                return m_ft;
        }
};

class Face final
{
        FT_Face m_face;

public:
        Face(const Face&) = delete;
        Face& operator=(const Face&) = delete;
        Face(Face&&) = delete;
        Face& operator=(Face&&) = delete;

        // Face(FT_Library ft, const char* font_file)
        //{
        //        if (FT_New_Face(ft, font_file, 0, &m_face))
        //        {
        //                error(std::string("Error open font file ") + font_file);
        //        }
        //}
        Face(FT_Library ft, const FT_Byte* memory_font, FT_Long memory_font_size)
        {
                if (FT_New_Memory_Face(ft, memory_font, memory_font_size, 0, &m_face))
                {
                        error("Error open memory font");
                }
        }
        ~Face()
        {
                FT_Done_Face(m_face);
        }
        FT_Face& get()
        {
                return m_face;
        }
};

struct TextVertex final
{
        float v1, v2; // Координаты вершины в пространстве.
        float t1, t2; // Координаты вершины в текстуре.
        TextVertex()
        {
        }
        TextVertex(float v1_, float v2_, float t1_, float t2_) : v1(v1_), v2(v2_), t1(t1_), t2(t2_)
        {
        }
};
}

class Text::Impl final
{
        Library m_ft;
        Face m_face;
        int m_size;

        VertexArray m_vertex_array;
        ArrayBuffer m_vertex_buffer;
        std::vector<TextVertex> m_vertices;
        GraphicsProgram m_program;
        ColorSpaceConverter m_color_converter;

        struct CharData final
        {
                int w, h, left, top, advance;
                TextureR32F tex;
                GLuint64 texture_handle;
                CharData(int w_, int h_, int left_, int top_, int advance_, const unsigned char* buf)
                        : w(w_),
                          h(h_),
                          left(left_),
                          top(top_),
                          advance(advance_),
                          tex(w, h, buf),
                          texture_handle(tex.get_texture().get_texture_resident_handle())
                {
                }
        };
        std::unordered_map<char, CharData> m_chars;

public:
        Impl()
                : m_face(m_ft.get(), font_bytes, sizeof(font_bytes)),
                  m_size(-1),
                  m_vertices(4),
                  m_program(VertexShader(text_vertex_shader), FragmentShader(text_fragment_shader)),
                  m_color_converter(true)
        {
                m_vertex_array.attrib_pointer(0, 3, GL_FLOAT, m_vertex_buffer, offsetof(TextVertex, v1), sizeof(TextVertex),
                                              true);
                m_vertex_array.attrib_pointer(1, 2, GL_FLOAT, m_vertex_buffer, offsetof(TextVertex, t1), sizeof(TextVertex),
                                              true);
                set_size(FONT_SIZE);
        }
        ~Impl()
        {
        }

        void set_color(const vec3& color)
        {
                m_program.set_uniform("text_color", to_vector<float>(color));
        }

        void set_size(int size)
        {
                FT_Set_Pixel_Sizes(m_face.get(), 0, m_size = size);
                m_chars.clear();
        }

        void draw(int width, int height, const std::vector<std::string>& text)
        {
                m_vertex_array.bind();

                float sx = 2.0f / width;
                float sy = 2.0f / height;

                float x = START_X;
                float y = START_Y;

                for (const std::string& line : text)
                {
                        for (char c : line)
                        {
                                auto i = m_chars.find(c);
                                if (i == m_chars.end())
                                {
                                        const unsigned char* buf;
                                        int w, h, left, top, advance;
                                        render_char(c, &buf, &w, &h, &left, &top, &advance);
                                        CharData cd(w, h, left, top, advance, buf);
                                        i = m_chars.emplace(c, std::move(cd)).first;

                                        // преобразование sRGB в RGB
                                        m_color_converter.convert((i->second).tex.get_texture());
                                }

                                m_program.set_uniform_handle("tex", (i->second).texture_handle);

                                int w = (i->second).w;
                                int h = (i->second).h;
                                int left = (i->second).left;
                                int top = (i->second).top;
                                int advance = (i->second).advance;

                                float x2 = -1.0f + (x + left) * sx;
                                float y2 = 1.0f - (y - top) * sy;

                                m_vertices[0] = TextVertex(x2, y2, 0, 0);
                                m_vertices[1] = TextVertex(x2 + w * sx, y2, 1, 0);
                                m_vertices[2] = TextVertex(x2, y2 - h * sy, 0, 1);
                                m_vertices[3] = TextVertex(x2 + w * sx, y2 - h * sy, 1, 1);

                                m_vertex_buffer.load_dynamic_draw(m_vertices);

                                m_program.draw_arrays(GL_TRIANGLE_STRIP, 0, m_vertices.size());

                                x += advance;
                        }

                        y += STEP_Y;
                        x = START_X;
                }
        }

        void render_char(char c, const unsigned char** buffer, int* w, int* h, int* left, int* top, int* advance_x)
        {
                if (m_size <= 0)
                {
                        error("Font size is not set");
                }
                if (c < 32 || c > 126)
                {
                        error("Only ASCII printable characters are supported in OpenGL text");
                }
                if (FT_Load_Char(m_face.get(), c, FT_LOAD_RENDER))
                {
                        error(std::string("Error load character ") + c);
                }
                *buffer = m_face.get()->glyph->bitmap.buffer;
                *w = m_face.get()->glyph->bitmap.width;
                *h = m_face.get()->glyph->bitmap.rows;
                *left = m_face.get()->glyph->bitmap_left;
                *top = m_face.get()->glyph->bitmap_top;
                *advance_x = m_face.get()->glyph->advance.x / 64;
        }

        void render_to_file(char c)
        {
                const unsigned char* buffer;
                int w, h, left, top, advance_x;
                render_char(c, &buffer, &w, &h, &left, &top, &advance_x);
                std::vector<unsigned char> image_buffer(w * h * 4);
                for (int i = 0; i < w * h; ++i)
                {
                        image_buffer[i * 4 + 0] = buffer[i]; // red
                        image_buffer[i * 4 + 1] = buffer[i]; // green
                        image_buffer[i * 4 + 2] = 255; // blue
                        image_buffer[i * 4 + 3] = 255; // alpha
                }
                sf::Image image;
                image.create(w, h, image_buffer.data());
                std::ostringstream o;
                o << "char=" << c << " w=" << w << " h=" << h << " left =" << left << " top=" << top
                  << " advance_x=" << advance_x;
                image.saveToFile(o.str() + ".png");
        }
};

Text::Text() : m_impl(std::make_unique<Impl>())
{
}
Text::~Text() = default;

void Text::set_size(int size)
{
        m_impl->set_size(size);
}
void Text::render_char(char c, const unsigned char** buffer, int* w, int* h, int* left, int* top, int* advance_x)
{
        m_impl->render_char(c, buffer, w, h, left, top, advance_x);
}
void Text::render_to_file(char c)
{
        m_impl->render_to_file(c);
}
void Text::set_color(const vec3& color)
{
        m_impl->set_color(color);
}
void Text::draw(int width, int height, const std::vector<std::string>& text)
{
        m_impl->draw(width, height, text);
}
