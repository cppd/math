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

#include "font.h"

#include "com/error.h"

#include <SFML/Graphics/Image.hpp>
#include <fstream>
#include <ft2build.h>
#include <iomanip>
#include <sstream>
#include <thread>
#include <type_traits>
#include <vector>

#include FT_FREETYPE_H

// clang-format off
constexpr const FT_Byte font_bytes[]
{
#include "DejaVuSans.ttf.bin"
};
// clang-format on

constexpr int MIN_CHAR = 32;
constexpr int MAX_CHAR = 126;

namespace
{
constexpr int to_int(char c)
{
        return static_cast<unsigned char>(c);
}

void check_char(char c)
{
        if (to_int(c) < MIN_CHAR || to_int(c) > MAX_CHAR)
        {
                error("Only ASCII printable characters are supported");
        }
}

class Library final
{
        FT_Library m_library;

public:
        Library()
        {
                if (FT_Init_FreeType(&m_library))
                {
                        error("Error init FreeType library");
                }
        }
        ~Library()
        {
                FT_Done_FreeType(m_library);
        }

        operator FT_Library() const
        {
                static_assert(std::is_pointer_v<FT_Library>);
                return m_library;
        }

        Library(const Library&) = delete;
        Library& operator=(const Library&) = delete;
        Library(Library&&) = delete;
        Library& operator=(Library&&) = delete;
};

class Face final
{
        FT_Face m_face;

public:
#if 0
        Face(FT_Library library, const std::string& font_file)
        {
                if (FT_New_Face(library, font_file.c_str(), 0, &m_face))
                {
                        error("Error FreeType new face, file " + font_file);
                }
        }
#endif
        Face(FT_Library library, const FT_Byte* memory_font, FT_Long memory_font_size)
        {
                if (FT_New_Memory_Face(library, memory_font, memory_font_size, 0, &m_face))
                {
                        error("Error FreeType new memory face");
                }
        }
        ~Face()
        {
                FT_Done_Face(m_face);
        }

        operator FT_Face() const
        {
                static_assert(std::is_pointer_v<FT_Face>);
                return m_face;
        }

        FT_Face operator->() const
        {
                static_assert(std::is_pointer_v<FT_Face>);
                return m_face;
        }

        Face(const Face&) = delete;
        Face& operator=(const Face&) = delete;
        Face(Face&&) = delete;
        Face& operator=(Face&&) = delete;
};

void save_to_file(Font::Char data)
{
        std::ostringstream oss;
        oss << std::setfill('0');
        oss << "char=" << std::setw(3) << to_int(data.c);
        oss << " size=" << data.size;
        oss << " w=" << data.width;
        oss << " h=" << data.height;
        oss << " left=" << data.left;
        oss << " top=" << data.top;
        oss << " advance_x=" << data.advance_x;

        if (data.width * data.height == 0)
        {
                if (data.c != ' ')
                {
                        error(std::string("No image for character '") + data.c + "'");
                }

                // Создать пустой файл
                oss << ".txt";
                std::ofstream f(oss.str());

                return;
        }

        oss << ".png";
        std::vector<sf::Uint8> image_buffer(data.width * data.height * 4);
        for (size_t i_dst = 0, i_src = 0; i_dst < image_buffer.size(); i_dst += 4, ++i_src)
        {
                image_buffer[i_dst + 0] = data.image[i_src]; // red
                image_buffer[i_dst + 1] = data.image[i_src]; // green
                image_buffer[i_dst + 2] = 255; // blue
                image_buffer[i_dst + 3] = 255; // alpha
        }
        sf::Image image;
        image.create(data.width, data.height, image_buffer.data());
        if (!image.saveToFile(oss.str()))
        {
                error(std::string("Error saving '") + data.c + "' image to file " + oss.str());
        }
}
}

class Font::Impl final
{
        const std::thread::id m_thread_id;

        Library m_library;
        Face m_face;

        int m_size;

public:
        Impl(int size_in_pixels) : m_thread_id(std::this_thread::get_id()), m_face(m_library, font_bytes, sizeof(font_bytes))
        {
                set_size(size_in_pixels);
        }
        ~Impl()
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);
        }

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;

        void set_size(int size_in_pixels)
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_size = size_in_pixels;
                FT_Set_Pixel_Sizes(m_face, 0, size_in_pixels);
        }

        Char render_char(char c)
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                check_char(c);

                if (FT_Load_Char(m_face, c, FT_LOAD_RENDER))
                {
                        error(std::string("FreeType failed to load and render character '") + c + "'");
                }

                Char res;

                res.c = c;
                res.image = m_face->glyph->bitmap.buffer;
                res.size = m_size;
                res.width = m_face->glyph->bitmap.width;
                res.height = m_face->glyph->bitmap.rows;
                res.left = m_face->glyph->bitmap_left;
                res.top = m_face->glyph->bitmap_top;
                res.advance_x = m_face->glyph->advance.x / 64;

                return res;
        }

        void render_all_to_files()
        {
                for (int i = MIN_CHAR; i <= MAX_CHAR; ++i)
                {
                        save_to_file(render_char(i));
                }
        }
};

Font::Font(int size_in_pixels) : m_impl(std::make_unique<Impl>(size_in_pixels))
{
}

Font::~Font() = default;

void Font::set_size(int size_in_pixels)
{
        m_impl->set_size(size_in_pixels);
}

Font::Char Font::render_char(char c)
{
        return m_impl->render_char(c);
}
