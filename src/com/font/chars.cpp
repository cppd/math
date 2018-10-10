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

#include "chars.h"

#include "com/alg.h"
#include "com/error.h"

#include <SFML/Graphics/Image.hpp>
#include <numeric>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
void save_grayscale_image_to_file[[maybe_unused]](const std::string& file_name, int width, int height,
                                                  const std::vector<std::uint_least8_t>& pixels)
{
        ASSERT(1ull * width * height == pixels.size());

        std::vector<sf::Uint8> buffer(4 * pixels.size());
        for (size_t buffer_i = 0, i = 0; i < pixels.size(); ++i)
        {
                buffer[buffer_i++] = pixels[i];
                buffer[buffer_i++] = pixels[i];
                buffer[buffer_i++] = pixels[i];
                buffer[buffer_i++] = 255;
        }

        sf::Image image;
        image.create(width, height, buffer.data());
        if (!image.saveToFile(file_name))
        {
                error("Error saving char texture to the file " + file_name);
        }
}

template <size_t... I>
bool check[[maybe_unused]](const std::array<int, sizeof...(I)>& offset, const std::array<int, sizeof...(I)>& copy_size,
                           const std::array<int, sizeof...(I)>& size, std::integer_sequence<size_t, I...>&&)
{
        return ((offset[I] >= 0) && ...) && ((copy_size[I] >= 0) && ...) && ((size[I] >= 0) && ...) &&
               ((offset[I] + copy_size[I] <= size[I]) && ...);
}

template <size_t N>
bool check[[maybe_unused]](const std::array<int, N>& offset, const std::array<int, N>& copy_size, const std::array<int, N>& size)
{
        return check(offset, copy_size, size, std::make_integer_sequence<size_t, N>());
}

//

template <typename T>
void copy_image(std::vector<T>* dst, const std::array<int, 2>& dst_size, const std::array<int, 2>& dst_offset,
                const std::vector<T>& src, const std::array<int, 2>& src_size, const std::array<int, 2>& src_offset,
                const std::array<int, 2>& copy_size)
{
        ASSERT(dst);
        ASSERT(src.size() == std::accumulate(src_size.begin(), src_size.end(), 1ull, std::multiplies<unsigned long long>()));
        ASSERT(dst->size() == std::accumulate(dst_size.begin(), dst_size.end(), 1ull, std::multiplies<unsigned long long>()));
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
void copy_image(std::vector<T>* dst, const std::array<int, 2>& dst_size, const std::array<int, 2>& dst_offset,
                const std::vector<T>& src, const std::array<int, 2>& src_size)
{
        copy_image(dst, dst_size, dst_offset, src, src_size, {0, 0}, src_size);
}

struct CharInfo
{
        FontChar font_char;
        std::vector<std::uint_least8_t> pixels;
        std::array<int, 2> coordinates;
};

void make_chars_and_coordinates(Font& font, int max_width, int max_height, std::unordered_map<char, CharInfo>* char_infos,
                                int* width, int* height)
{
        char_infos->clear();
        *width = 0;
        *height = 0;

        int row_height = 0, insert_x = 0, insert_y = 0;

        std::vector<char> supported_characters = font.supported_characters();
        sort_and_unique(&supported_characters);

        for (char c : supported_characters)
        {
                Font::Char rc = font.render_char(c);

                if (rc.width < 0 || rc.height < 0)
                {
                        error("Negative character size");
                }
                if ((rc.width <= 0 && rc.height > 0) || (rc.width > 0 && rc.height <= 0))
                {
                        error("One-dimensional character image");
                }
                if (rc.width > max_width)
                {
                        error("Character is too wide");
                }

                ASSERT(insert_x <= max_width);
                if (insert_x + rc.width > max_width || insert_x == max_width)
                {
                        ASSERT(row_height > 0);

                        insert_y += row_height;
                        insert_x = 0;
                        row_height = 0;
                }

                if (insert_y + rc.height > max_height)
                {
                        error("Maximum character image height limit");
                }

                //

                CharInfo info;

                info.font_char.left = rc.left;
                info.font_char.top = rc.top;
                info.font_char.width = rc.width;
                info.font_char.height = rc.height;
                info.font_char.advance_x = rc.advance_x;

                info.pixels.resize(1ull * rc.width * rc.height);
                for (size_t i = 0; i < info.pixels.size(); ++i)
                {
                        info.pixels[i] = rc.image[i];
                }
                info.coordinates = {insert_x, insert_y};

                char_infos->emplace(c, std::move(info));

                //

                *width = std::max(*width, insert_x + rc.width);
                *height = std::max(*height, insert_y + rc.height);

                insert_x += rc.width;
                row_height = std::max(row_height, rc.height);
        }
}

void make_texture_and_texture_coordinates(int width, int height, std::unordered_map<char, CharInfo>* chars,
                                          std::vector<std::uint_least8_t>* pixels)
{
        pixels->resize(1ll * width * height);

        float r_width = 1.0f / width;
        float r_height = 1.0f / height;

        for (auto& c : *chars)
        {
                CharInfo& info = c.second;

                copy_image(pixels, {width, height}, info.coordinates, info.pixels, {info.font_char.width, info.font_char.height});

                ASSERT(info.coordinates[0] >= 0 && info.coordinates[0] + info.font_char.width <= width);
                ASSERT(info.coordinates[1] >= 0 && info.coordinates[1] + info.font_char.height <= height);

                info.font_char.texture_x = info.coordinates[0] * r_width;
                info.font_char.texture_y = info.coordinates[1] * r_height;
                info.font_char.texture_width = info.font_char.width * r_width;
                info.font_char.texture_height = info.font_char.height * r_height;
        }
}
}

void create_font_chars(Font& font, unsigned max_width, unsigned max_height, std::unordered_map<char, FontChar>* chars,
                       int* texture_width, int* texture_height, std::vector<std::uint_least8_t>* texture_pixels)
{
        std::unordered_map<char, CharInfo> char_info;

        make_chars_and_coordinates(font, max_width, max_height, &char_info, texture_width, texture_height);

        make_texture_and_texture_coordinates(*texture_width, *texture_height, &char_info, texture_pixels);

#if 0
        save_grayscale_image_to_file("font_texture.png", *texture_width, *texture_height, *texture_pixels);
#endif

        chars->clear();
        chars->reserve(char_info.size());
        for (const auto& [c, info] : char_info)
        {
                chars->emplace(c, info.font_char);
        }
}
