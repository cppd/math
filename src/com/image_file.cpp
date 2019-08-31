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

#include "image_file.h"

#include "error.h"

#include "com/file/file_sys.h"

#include <SFML/Graphics/Image.hpp>
#include <cstring>

namespace
{
std::string file_name_with_extension(const std::string& file_name)
{
        std::string ext = file_extension(file_name);

        if (ext.size() > 0)
        {
                return file_name;
        }

        // Если имя заканчивается на точку, то пусть будет 2 точки подряд
        return file_name + ".png";
}
}

void save_grayscale_image_to_file(const std::string& file_name, int width, int height, Span<const std::uint_least8_t> pixels)
{
        if (1ull * width * height != pixels.size())
        {
                error("Error image size");
        }

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
        std::string f = file_name_with_extension(file_name);
        if (!image.saveToFile(f))
        {
                error("Error saving pixels to the file " + f);
        }
}

void save_srgb_image_to_file(const std::string& file_name, int width, int height, const std::vector<std::uint_least8_t>& pixels)
{
        if (3ull * width * height != pixels.size())
        {
                error("Error image size");
        }

        std::vector<sf::Uint8> buffer(4 * pixels.size());
        for (size_t buffer_i = 0, image_i = 0; image_i < pixels.size();)
        {
                buffer[buffer_i++] = pixels[image_i++];
                buffer[buffer_i++] = pixels[image_i++];
                buffer[buffer_i++] = pixels[image_i++];
                buffer[buffer_i++] = 255;
        }

        sf::Image image;
        image.create(width, height, buffer.data());
        std::string f = file_name_with_extension(file_name);
        if (!image.saveToFile(f))
        {
                error("Error saving pixels to the file " + f);
        }
}

void save_srgba_image_to_file(const std::string& file_name, int width, int height, const std::vector<std::uint_least8_t>& pixels)
{
        if (4ull * width * height != pixels.size())
        {
                error("Error image size");
        }

        sf::Image image;
        image.create(width, height, pixels.data());
        std::string f = file_name_with_extension(file_name);
        if (!image.saveToFile(f))
        {
                error("Error saving pixels to the file " + f);
        }
}

void load_srgba_image_from_file(const std::string& file_name, int* width, int* height, std::vector<std::uint_least8_t>* pixels)
{
        sf::Image image;
        if (!image.loadFromFile(file_name))
        {
                error("Error load image from file " + file_name);
        }

        *width = image.getSize().x;
        *height = image.getSize().y;

        unsigned long long buffer_size = 4ull * image.getSize().x * image.getSize().y;
        pixels->resize(buffer_size);

        static_assert(sizeof(decltype(*pixels->data())) == sizeof(decltype(*image.getPixelsPtr())));
        std::memcpy(pixels->data(), image.getPixelsPtr(), buffer_size);
}

void flip_srgba_image_vertically(int width, int height, std::vector<std::uint_least8_t>* pixels)
{
        if (4ull * width * height != pixels->size())
        {
                error("Error image size");
        }

        size_t row_size = 4ull * width;
        size_t row_end = (height / 2) * row_size;
        for (size_t row1 = 0, row2 = (height - 1) * row_size; row1 < row_end; row1 += row_size, row2 -= row_size)
        {
                for (size_t i = 0; i < row_size; ++i)
                {
                        std::swap((*pixels)[row1 + i], (*pixels)[row2 + i]);
                }
        }
}
