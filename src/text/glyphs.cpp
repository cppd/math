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

#include "glyphs.h"

#include "code_points.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/image/file.h>

#include <array>
#include <cstring>
#include <numeric>
#include <utility>

namespace ns::text
{
namespace
{
template <std::size_t... I>
[[maybe_unused]] bool check(
        const std::array<int, sizeof...(I)>& offset,
        const std::array<int, sizeof...(I)>& copy_size,
        const std::array<int, sizeof...(I)>& size,
        std::integer_sequence<std::size_t, I...>&&)
{
        return ((offset[I] >= 0) && ...) && ((copy_size[I] >= 0) && ...) && ((size[I] >= 0) && ...)
               && ((offset[I] + copy_size[I] <= size[I]) && ...);
}

template <std::size_t N>
[[maybe_unused]] bool check(
        const std::array<int, N>& offset,
        const std::array<int, N>& copy_size,
        const std::array<int, N>& size)
{
        return check(offset, copy_size, size, std::make_integer_sequence<std::size_t, N>());
}

//

template <typename T>
void copy_image(
        std::vector<T>* dst,
        const std::array<int, 2>& dst_size,
        const std::array<int, 2>& dst_offset,
        const std::vector<T>& src,
        const std::array<int, 2>& src_size,
        const std::array<int, 2>& src_offset,
        const std::array<int, 2>& copy_size)
{
        ASSERT(dst);
        ASSERT(src.size()
               == std::accumulate(src_size.begin(), src_size.end(), 1ull, std::multiplies<unsigned long long>()));
        ASSERT(dst->size()
               == std::accumulate(dst_size.begin(), dst_size.end(), 1ull, std::multiplies<unsigned long long>()));
        ASSERT(check(src_offset, copy_size, src_size));
        ASSERT(check(dst_offset, copy_size, dst_size));

        long long dst_y_stride = dst_size[0];
        long long src_y_stride = src_size[0];

        for (int s_y = src_offset[1], d_y = dst_offset[1]; s_y < src_offset[1] + copy_size[1]; ++s_y, ++d_y)
        {
                for (int s_x = src_offset[0], d_x = dst_offset[0]; s_x < src_offset[0] + copy_size[0]; ++s_x, ++d_x)
                {
                        (*dst)[d_y * dst_y_stride + d_x] = src[s_y * src_y_stride + s_x];
                }
        }
}

template <typename T>
void copy_image(
        std::vector<T>* dst,
        const std::array<int, 2>& dst_size,
        const std::array<int, 2>& dst_offset,
        const std::vector<T>& src,
        const std::array<int, 2>& src_size)
{
        copy_image(dst, dst_size, dst_offset, src, src_size, {0, 0}, src_size);
}

void render_glyphs(
        const std::vector<char32_t>& code_points,
        const Font& font,
        std::unordered_map<char32_t, FontGlyph>* font_glyphs,
        std::unordered_map<char32_t, std::vector<std::byte>>* glyph_pixels)
{
        font_glyphs->clear();
        glyph_pixels->clear();

        font_glyphs->reserve(code_points.size());
        glyph_pixels->reserve(code_points.size());

        for (char32_t code_point : code_points)
        {
                std::optional<Font::Char> rc = font.render(code_point);

                if (!rc)
                {
                        continue;
                }

                if (rc->width < 0 || rc->height < 0)
                {
                        error("Negative character size");
                }
                if ((rc->width <= 0 && rc->height > 0) || (rc->width > 0 && rc->height <= 0))
                {
                        error("One-dimensional character image");
                }

                FontGlyph& font_glyph = font_glyphs->try_emplace(code_point).first->second;

                font_glyph.left = rc->left;
                font_glyph.top = rc->top;
                font_glyph.width = rc->width;
                font_glyph.height = rc->height;
                font_glyph.advance_x = rc->advance_x;

                static_assert(std::is_same_v<const unsigned char*, decltype(rc->image)>);

                std::vector<std::byte>& pixels = glyph_pixels->try_emplace(code_point).first->second;
                pixels.resize(1ull * rc->width * rc->height);
                std::memcpy(data_pointer(pixels), rc->image, data_size(pixels));
        }
}

template <typename Key, typename Value>
void place_rectangles_on_rectangle(
        const std::unordered_map<Key, Value>& rectangles,
        int max_rectangle_width,
        int max_rectangle_height,
        int* rectangle_width,
        int* rectangle_height,
        std::unordered_map<Key, std::array<int, 2>>* rectangle_coordinates)
{
        rectangle_coordinates->clear();
        rectangle_coordinates->reserve(rectangles.size());

        *rectangle_width = 0;
        *rectangle_height = 0;

        int row_height = 0;
        int insert_x = 0;
        int insert_y = 0;

        for (const auto& [key, value] : rectangles)
        {
                ASSERT(insert_x <= max_rectangle_width);

                if (insert_x + value.width > max_rectangle_width || insert_x == max_rectangle_width)
                {
                        ASSERT(row_height > 0);

                        insert_y += row_height;
                        insert_x = 0;
                        row_height = 0;
                }

                if (insert_x + value.width > max_rectangle_width)
                {
                        error("Maximum rectangle width exceeded");
                }
                if (insert_y + value.height > max_rectangle_height)
                {
                        error("Maximum rectangle height exceeded");
                }

                rectangle_coordinates->emplace(key, std::array<int, 2>{insert_x, insert_y});

                *rectangle_width = std::max(*rectangle_width, insert_x + value.width);
                *rectangle_height = std::max(*rectangle_height, insert_y + value.height);

                insert_x += value.width;
                row_height = std::max(row_height, value.height);
        }
}

void fill_texture_pixels_and_texture_coordinates(
        int texture_width,
        int texture_height,
        const std::unordered_map<char32_t, std::vector<std::byte>>& glyph_pixels,
        const std::unordered_map<char32_t, std::array<int, 2>>& glyph_coordinates,
        std::unordered_map<char32_t, FontGlyph>* font_glyphs,
        std::vector<std::byte>* texture_pixels)
{
        texture_pixels->clear();
        texture_pixels->resize(1ull * texture_width * texture_height);
        std::memset(texture_pixels->data(), 0, texture_pixels->size());

        float r_width = 1.0f / texture_width;
        float r_height = 1.0f / texture_height;

        for (auto& [cp, font_glyph] : *font_glyphs)
        {
                ASSERT(glyph_pixels.count(cp) == 1);
                ASSERT(glyph_coordinates.count(cp) == 1);

                const std::vector<std::byte>& pixels = glyph_pixels.find(cp)->second;
                const std::array<int, 2>& coordinates = glyph_coordinates.find(cp)->second;

                copy_image(
                        texture_pixels, {texture_width, texture_height}, coordinates, pixels,
                        {font_glyph.width, font_glyph.height});

                font_glyph.s0 = r_width * coordinates[0];
                font_glyph.s1 = r_width * (coordinates[0] + font_glyph.width);

                font_glyph.t0 = r_height * coordinates[1];
                font_glyph.t1 = r_height * (coordinates[1] + font_glyph.height);
        }
}
}

void create_font_glyphs(
        const Font& font,
        int max_width,
        int max_height,
        std::unordered_map<char32_t, FontGlyph>* font_glyphs,
        image::Image<2>* image)
{
        std::unordered_map<char32_t, std::vector<std::byte>> glyph_pixels;

        render_glyphs(supported_code_points(), font, font_glyphs, &glyph_pixels);

        std::unordered_map<char32_t, std::array<int, 2>> glyph_coordinates;

        place_rectangles_on_rectangle(
                *font_glyphs, max_width, max_height, &image->size[0], &image->size[1], &glyph_coordinates);

        fill_texture_pixels_and_texture_coordinates(
                image->size[0], image->size[1], glyph_pixels, glyph_coordinates, font_glyphs, &image->pixels);

        image->color_format = image::ColorFormat::R8_SRGB;

        if ((false))
        {
                image::save(path_from_utf8("font_texture.png"), image::ImageView<2>(*image));
        }
}
}
