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

#include "file.h"

#include "com/error.h"

#include <SFML/Graphics/Image.hpp>

void save_grayscale_image_to_file(const std::string& file_name, int width, int height, Span<const std::uint_least8_t> pixels)
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
                error("Error saving pixels to the file " + file_name);
        }
}
