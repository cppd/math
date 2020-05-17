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

#include "font.h"

#include "unicode.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/image/file.h>

#include <cmath>
#include <fstream>
#include <ft2build.h>
#include <sstream>
#include <thread>
#include <type_traits>

#include FT_FREETYPE_H

namespace text
{
namespace
{
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
        std::vector<unsigned char> m_font_data;
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
        template <typename T>
        Face(FT_Library library, T&& font_data) : m_font_data(std::forward<T>(font_data))
        {
                if (FT_New_Memory_Face(library, data_pointer(m_font_data), data_size(m_font_data), 0, &m_face))
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

void save_to_file(char32_t code_point, const std::optional<Font::Char>& data)
{
        if (!data)
        {
                std::ostringstream oss;
                oss << "code_point=" << unicode::utf32_to_number_string(code_point) << ".txt";
                // Создать пустой файл
                std::ofstream f(oss.str());
                if (!f)
                {
                        error("Error creating the file " + oss.str());
                }
                return;
        }

        ASSERT(code_point == data->code_point);

        std::ostringstream oss;
        oss << "code_point=" << unicode::utf32_to_number_string(data->code_point);
        oss << " size=" << data->size;
        oss << " w=" << data->width;
        oss << " h=" << data->height;
        oss << " left=" << data->left;
        oss << " top=" << data->top;
        oss << " advance_x=" << data->advance_x;

        if (data->width * data->height == 0)
        {
                // Создать пустой файл
                oss << ".txt";
                std::ofstream f(oss.str());
                if (!f)
                {
                        error("Error creating the file " + oss.str());
                }
                return;
        }

        oss << ".png";

        save_image_to_file(
                oss.str(), data->width, data->height, ColorFormat::R8_SRGB,
                Span<const std::byte>(
                        reinterpret_cast<const std::byte*>(data->image), 1ull * data->width * data->height));
}
}

class Font::Impl final
{
        const std::thread::id m_thread_id;

        Library m_library;
        Face m_face;

        int m_size;

public:
        template <typename T>
        Impl(int size_in_pixels, T&& font_data)
                : m_thread_id(std::this_thread::get_id()), m_face(m_library, std::forward<T>(font_data))
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

        std::optional<Char> render(char32_t code_point) const
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (FT_Load_Char(m_face, code_point, FT_LOAD_RENDER))
                {
                        return std::nullopt;
                }

                Char res;

                res.code_point = code_point;
                res.image = m_face->glyph->bitmap.buffer;
                res.size = m_size;
                res.width = m_face->glyph->bitmap.width;
                res.height = m_face->glyph->bitmap.rows;
                res.left = m_face->glyph->bitmap_left;
                res.top = m_face->glyph->bitmap_top;
                res.advance_x = std::lround(m_face->glyph->advance.x / 64.0);

                return res;
        }

        void render_ascii_printable_characters_to_files()
        {
                for (char32_t code_point = 32; code_point <= 126; ++code_point)
                {
                        save_to_file(code_point, render(code_point));
                }
        }
};

Font::Font(int size_in_pixels, std::vector<unsigned char>&& font_data)
        : m_impl(std::make_unique<Impl>(size_in_pixels, std::move(font_data)))
{
}

Font::~Font() = default;

void Font::set_size(int size_in_pixels)
{
        m_impl->set_size(size_in_pixels);
}

template <typename T>
std::enable_if_t<std::is_same_v<T, char32_t>, std::optional<Font::Char>> Font::render(T code_point) const
{
        return m_impl->render(code_point);
}

template std::optional<Font::Char> Font::render(char32_t code_point) const;
}
