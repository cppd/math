/*
Copyright (C) 2017-2023 Topological Manifold

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
#include <src/com/file/path.h>
#include <src/image/file_save.h>
#include <src/image/format.h>
#include <src/image/image.h>
#include <src/settings/directory.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <ft2build.h>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include FT_FREETYPE_H

namespace ns::text
{
namespace
{
class Library final
{
        FT_Library library_;

public:
        Library()
        {
                if (FT_Init_FreeType(&library_))
                {
                        error("Error init FreeType library");
                }
        }

        ~Library()
        {
                FT_Done_FreeType(library_);
        }

        [[nodiscard]] FT_Library get() const noexcept
        {
                static_assert(std::is_pointer_v<FT_Library>);
                return library_;
        }

        Library(const Library&) = delete;
        Library& operator=(const Library&) = delete;
        Library(Library&&) = delete;
        Library& operator=(Library&&) = delete;
};

class Face final
{
        std::vector<unsigned char> font_data_;
        FT_Face face_;

public:
        template <typename T>
        Face(const FT_Library library, T&& font_data)
                : font_data_(std::forward<T>(font_data))
        {
                if (FT_New_Memory_Face(library, data_pointer(font_data_), data_size(font_data_), 0, &face_))
                {
                        error("Error FreeType new memory face");
                }
        }

        ~Face()
        {
                FT_Done_Face(face_);
        }

        [[nodiscard]] FT_Face get() const noexcept
        {
                static_assert(std::is_pointer_v<FT_Face>);
                return face_;
        }

        [[nodiscard]] FT_Face operator->() const noexcept
        {
                static_assert(std::is_pointer_v<FT_Face>);
                return face_;
        }

        Face(const Face&) = delete;
        Face& operator=(const Face&) = delete;
        Face(Face&&) = delete;
        Face& operator=(Face&&) = delete;
};

std::filesystem::path character_file_path(const std::string& file_name)
{
        return settings::test_directory() / path_from_utf8(file_name);
}

void create_empty_file(const std::string& file_name)
{
        const std::filesystem::path path = character_file_path(file_name);
        const std::ofstream file(path);
        if (!file)
        {
                error("Error creating the file " + generic_utf8_filename(path));
        }
}

void save_to_file(const char32_t code_point, const std::optional<Char>& data)
{
        if (!data)
        {
                create_empty_file("code_point=" + unicode::utf32_to_number_string(code_point) + ".txt");
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
                oss << ".txt";
                create_empty_file(oss.str());
                return;
        }

        oss << ".png";
        image::save(
                character_file_path(oss.str()),
                image::ImageView<2>(
                        {data->width, data->height}, image::ColorFormat::R8_SRGB,
                        std::as_bytes(std::span(data->image, 1ull * data->width * data->height))));
}

class Impl final : public Font
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        Library library_;
        Face face_;

        int size_;

        void set_size(const int size_in_pixels) override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                size_ = size_in_pixels;
                FT_Set_Pixel_Sizes(face_.get(), 0, size_in_pixels);
        }

        [[nodiscard]] std::optional<Char> render_impl(const char32_t code_point) const override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                if (FT_Load_Char(face_.get(), code_point, FT_LOAD_RENDER))
                {
                        return std::nullopt;
                }

                Char res;

                res.code_point = code_point;
                res.image = face_->glyph->bitmap.buffer;
                res.size = size_;
                res.width = face_->glyph->bitmap.width;
                res.height = face_->glyph->bitmap.rows;
                res.left = face_->glyph->bitmap_left;
                res.top = face_->glyph->bitmap_top;
                res.advance_x = std::lround(face_->glyph->advance.x / 64.0);

                return res;
        }

        void render_ascii_printable_characters_to_files() const override
        {
                for (char32_t code_point = 32; code_point <= 126; ++code_point)
                {
                        save_to_file(code_point, render(code_point));
                }
        }

public:
        Impl(const int size_in_pixels, std::vector<unsigned char>&& font_data)
                : face_(library_.get(), std::move(font_data))
        {
                set_size(size_in_pixels);
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);
        }
};
}

std::unique_ptr<Font> create_font(const int size_in_pixels, std::vector<unsigned char>&& font_data)
{
        return std::make_unique<Impl>(size_in_pixels, std::move(font_data));
}
}
