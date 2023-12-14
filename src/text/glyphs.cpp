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

#include "glyphs.h"

#include "code_points.h"
#include "font.h"

#include <src/com/alg.h>
#include <src/com/container.h>
#include <src/com/error.h>
#include <src/image/format.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ns::text
{
namespace
{
template <std::size_t N>
[[maybe_unused]] bool check(
        const std::array<int, N>& offset,
        const std::array<int, N>& copy_size,
        const std::array<int, N>& size)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!(offset[i] >= 0 && copy_size[i] >= 0 && size[i] >= 0 && offset[i] + copy_size[i] <= size[i]))
                {
                        return false;
                }
        }
        return true;
}

template <typename T>
void copy_image(
        std::vector<T>* const dst,
        const std::array<int, 2>& dst_size,
        const std::array<int, 2>& dst_offset,
        const std::vector<T>& src,
        const std::array<int, 2>& src_size,
        const std::array<int, 2>& src_offset,
        const std::array<int, 2>& copy_size)
{
        ASSERT(dst);
        ASSERT(static_cast<long long>(src.size()) == multiply_all<long long>(src_size));
        ASSERT(static_cast<long long>(dst->size()) == multiply_all<long long>(dst_size));
        ASSERT(check(src_offset, copy_size, src_size));
        ASSERT(check(dst_offset, copy_size, dst_size));

        const long long dst_y_stride = dst_size[0];
        const long long src_y_stride = src_size[0];

        for (int s_y = src_offset[1], d_y = dst_offset[1]; s_y < src_offset[1] + copy_size[1]; ++s_y, ++d_y)
        {
                for (int s_x = src_offset[0], d_x = dst_offset[0]; s_x < src_offset[0] + copy_size[0]; ++s_x, ++d_x)
                {
                        (*dst)[d_y * dst_y_stride + d_x] = src[s_y * src_y_stride + s_x];
                }
        }
}

struct RenderedGlyph final
{
        std::unordered_map<char32_t, FontGlyph> font_glyphs;
        std::unordered_map<char32_t, std::vector<std::byte>> glyph_pixels;
};

RenderedGlyph render_glyphs(const std::vector<char32_t>& code_points, const Font& font)
{
        RenderedGlyph res;

        res.font_glyphs.reserve(code_points.size());
        res.glyph_pixels.reserve(code_points.size());

        for (const char32_t code_point : code_points)
        {
                const std::optional<Char> font_char = font.render(code_point);

                if (!font_char)
                {
                        continue;
                }

                if (font_char->width < 0 || font_char->height < 0)
                {
                        error("Negative character size");
                }

                if ((font_char->width <= 0 && font_char->height > 0)
                    || (font_char->width > 0 && font_char->height <= 0))
                {
                        error("One-dimensional character image");
                }

                FontGlyph& font_glyph = res.font_glyphs.try_emplace(code_point).first->second;

                font_glyph.left = font_char->left;
                font_glyph.top = font_char->top;
                font_glyph.width = font_char->width;
                font_glyph.height = font_char->height;
                font_glyph.advance_x = font_char->advance_x;

                static_assert(std::is_same_v<const unsigned char*, decltype(font_char->image)>);

                std::vector<std::byte>& pixels = res.glyph_pixels.try_emplace(code_point).first->second;
                pixels.resize(1ull * font_char->width * font_char->height);
                std::memcpy(data_pointer(pixels), font_char->image, data_size(pixels));
        }

        return res;
}

struct PlacedRectangles final
{
        int width;
        int height;
        std::unordered_map<char32_t, std::array<int, 2>> coordinates;
};

template <typename Key, typename Value>
PlacedRectangles place_rectangles_on_rectangle(
        const std::unordered_map<Key, Value>& rectangles,
        const int max_rectangle_width,
        const int max_rectangle_height)
{
        PlacedRectangles res;

        res.width = 0;
        res.height = 0;
        res.coordinates.reserve(rectangles.size());

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

                res.coordinates.emplace(key, std::array<int, 2>{insert_x, insert_y});

                res.width = std::max(res.width, insert_x + value.width);
                res.height = std::max(res.height, insert_y + value.height);

                insert_x += value.width;
                row_height = std::max(row_height, value.height);
        }

        return res;
}

void fill_texture_pixels_and_texture_coordinates(
        const int texture_width,
        const int texture_height,
        const std::unordered_map<char32_t, std::vector<std::byte>>& glyph_pixels,
        const std::unordered_map<char32_t, std::array<int, 2>>& glyph_coordinates,
        std::unordered_map<char32_t, FontGlyph>* const font_glyphs,
        std::vector<std::byte>* const texture_pixels)
{
        texture_pixels->clear();
        texture_pixels->resize(static_cast<long long>(texture_width) * texture_height);
        std::memset(texture_pixels->data(), 0, texture_pixels->size());

        const float texture_width_f = texture_width;
        const float texture_height_f = texture_height;

        const std::array<int, 2> texture_size{texture_width, texture_height};

        for (auto& [cp, font_glyph] : *font_glyphs)
        {
                ASSERT(glyph_pixels.count(cp) == 1);
                ASSERT(glyph_coordinates.count(cp) == 1);

                const std::array<int, 2>& texture_offset = glyph_coordinates.find(cp)->second;
                const std::vector<std::byte>& pixels = glyph_pixels.find(cp)->second;
                const std::array<int, 2> size{font_glyph.width, font_glyph.height};
                const std::array<int, 2> offset{0, 0};

                copy_image(texture_pixels, texture_size, texture_offset, pixels, size, offset, size);

                font_glyph.s0 = texture_offset[0] / texture_width_f;
                font_glyph.s1 = (texture_offset[0] + font_glyph.width) / texture_width_f;

                font_glyph.t0 = texture_offset[1] / texture_height_f;
                font_glyph.t1 = (texture_offset[1] + font_glyph.height) / texture_height_f;
        }
}
}

FontGlyphs create_font_glyphs(const Font& font, const int max_width, const int max_height)
{
        RenderedGlyph rendered_glyphs = render_glyphs(supported_code_points(), font);

        const PlacedRectangles placed_rectangles =
                place_rectangles_on_rectangle(rendered_glyphs.font_glyphs, max_width, max_height);

        FontGlyphs res;

        res.glyphs = std::move(rendered_glyphs.font_glyphs);

        fill_texture_pixels_and_texture_coordinates(
                placed_rectangles.width, placed_rectangles.height, rendered_glyphs.glyph_pixels,
                placed_rectangles.coordinates, &res.glyphs, &res.image.pixels);

        res.image.color_format = image::ColorFormat::R8_SRGB;
        res.image.size[0] = placed_rectangles.width;
        res.image.size[1] = placed_rectangles.height;

        // image::save(settings::test_directory() / path_from_utf8("font_texture.png"), image::ImageView<2>(res.image));

        return res;
}
}
