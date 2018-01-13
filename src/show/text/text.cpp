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

#include <SFML/Graphics/Image.hpp>
#include <ft2build.h>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include FT_FREETYPE_H

constexpr int FONT_SIZE = 12;

constexpr float STEP_Y = 16;
constexpr float START_X = 10;
constexpr float START_Y = 20;

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

class CharData final
{
        TextureR32F texture;

public:
        const int w, h, left, top, advance;
        const GLuint64 texture_handle;

        CharData(int w_, int h_, int left_, int top_, int advance_, const std::vector<float>& pixels)
                : texture(w_, h_, pixels),
                  w(w_),
                  h(h_),
                  left(left_),
                  top(top_),
                  advance(advance_),
                  texture_handle(texture.get_texture().get_texture_resident_handle())
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
}

class Text::Impl final
{
        Library m_ft;
        Face m_face;
        int m_size;

        VertexArray m_vertex_array;
        ArrayBuffer m_vertex_buffer;
        GraphicsProgram m_program;

        std::unordered_map<char, CharData> m_chars;

        const CharData& get_char_data(char c)
        {
                auto iter = m_chars.find(c);

                if (iter == m_chars.end())
                {
                        const unsigned char* buffer;
                        int width, height, left, top, advance;

                        render_char(c, &buffer, &width, &height, &left, &top, &advance);

                        iter = m_chars.try_emplace(c, width, height, left, top, advance,
                                                   integer_pixels_to_float_pixels(width, height, buffer))
                                       .first;
                }

                return iter->second;
        }

public:
        Impl()
                : m_face(m_ft.get(), font_bytes, sizeof(font_bytes)),
                  m_size(-1),
                  m_program(VertexShader(text_vertex_shader), FragmentShader(text_fragment_shader))
        {
                m_vertex_array.attrib_pointer(0, 3, GL_FLOAT, m_vertex_buffer, offsetof(Vertex, v1), sizeof(Vertex), true);
                m_vertex_array.attrib_pointer(1, 2, GL_FLOAT, m_vertex_buffer, offsetof(Vertex, t1), sizeof(Vertex), true);
                set_size(FONT_SIZE);
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
                                const CharData& cd = get_char_data(c);

                                m_program.set_uniform_handle("tex", cd.texture_handle);

                                float x2 = -1.0f + (x + cd.left) * sx;
                                float y2 = 1.0f - (y - cd.top) * sy;

                                std::array<Vertex, 4> vertices;

                                vertices[0] = Vertex(x2, y2, 0, 0);
                                vertices[1] = Vertex(x2 + cd.w * sx, y2, 1, 0);
                                vertices[2] = Vertex(x2, y2 - cd.h * sy, 0, 1);
                                vertices[3] = Vertex(x2 + cd.w * sx, y2 - cd.h * sy, 1, 1);

                                m_vertex_buffer.load_dynamic_draw(vertices);
                                m_program.draw_arrays(GL_TRIANGLE_STRIP, 0, vertices.size());

                                x += cd.advance;
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
                for (size_t i = 0; i < image_buffer.size(); i += 4)
                {
                        image_buffer[i + 0] = buffer[i]; // red
                        image_buffer[i + 1] = buffer[i]; // green
                        image_buffer[i + 2] = 255; // blue
                        image_buffer[i + 3] = 255; // alpha
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
