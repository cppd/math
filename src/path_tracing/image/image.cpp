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

#include "image.h"

#include "com/colors.h"
#include "com/error.h"
#include "com/file.h"
#include "com/file_sys.h"
#include "com/interpolation.h"
#include "com/str.h"

#include <SFML/Graphics/Image.hpp>
#include <algorithm>

namespace
{
std::string file_name_with_extension(const std::string& file_name, const std::string& extension)
{
        std::string ext = to_lower(trim(get_extension(file_name)));
        if (ext.size() > 0)
        {
                if (ext != to_lower(trim(extension)))
                {
                        error("Unsupported image file format");
                }
                return file_name;
        }
        return file_name + "." + extension;
}
}

Image::Image(int width, int height)
{
        resize(width, height);
}

Image::Image(int width, int height, const std::vector<unsigned char>& srgba_pixels)
{
        ASSERT(4ull * width * height == srgba_pixels.size());

        read_from_srgba_pixels(width, height, srgba_pixels.data());
}

void Image::resize(int width, int height)
{
        if (m_width == width && m_height == height)
        {
                return;
        }

        m_data.clear();
        m_data.shrink_to_fit();

        m_width = std::max(width, 0);
        m_height = std::max(height, 0);

        m_max_x = m_width - 1;
        m_max_y = m_height - 1;

        m_max_x0 = m_width - 2;
        m_max_y0 = m_height - 2;

        m_data.resize(m_height * m_width);
}

int Image::width() const
{
        return m_width;
}

int Image::height() const
{
        return m_height;
}

bool Image::empty() const
{
        return m_data.size() == 0;
}

void Image::clear(const vec3& color)
{
        std::fill(m_data.begin(), m_data.end(), color);
}

void Image::set_pixel(int x, int y, const vec3& color)
{
        int index = y * m_width + x;
        m_data[index] = color;
}

const vec3& Image::get_pixel(int x, int y) const
{
        int index = y * m_width + x;
        return m_data[index];
}

vec3 Image::get_texture(const vec2& p) const
{
        double tx = std::clamp(p[0], 0.0, 1.0) * m_max_x;
        double ty = std::clamp(p[1], 0.0, 1.0) * m_max_y;

        // Интерполяция по 4 пикселям

        int x0 = static_cast<int>(tx);
        int y0 = static_cast<int>(ty);

        // Если значение tx или ty равно максимуму (это целое число), то x0 и y0 получатся
        // неправильными, поэтому требуется корректировка для этого случая.
        x0 = std::min(x0, m_max_x0);
        y0 = std::min(y0, m_max_y0);

        int x1 = x0 + 1;
        int y1 = y0 + 1;

        double local_x = tx - x0;
        double local_y = ty - y0;

        return interpolation(get_pixel(x0, y0), get_pixel(x1, y0), get_pixel(x0, y1), get_pixel(x1, y1), local_x, local_y);
}

void Image::read_from_srgba_pixels(int width, int height, const unsigned char* srgba_pixels)
{
        resize(width, height);

        for (size_t i = 0, p = 0; i < m_data.size(); p += 4, ++i)
        {
                m_data[i] = srgb_integer_to_rgb_float(srgba_pixels[p], srgba_pixels[p + 1], srgba_pixels[p + 2]);
        }
}

void Image::read_from_file(const std::string& file_name)
{
        sf::Image image;

        if (!image.loadFromFile(file_name))
        {
                error("Error read image from file " + file_name);
        }

        read_from_srgba_pixels(image.getSize().x, image.getSize().y, image.getPixelsPtr());
}

// Запись в формат PPM с цветом sRGB
void Image::write_to_file(const std::string& file_name) const
{
        if (empty())
        {
                error("No data to write the image to the file " + file_name);
        }

        CFile fp(file_name_with_extension(file_name, "ppm"), "wb");

        static_assert(std::is_same_v<decltype(m_width), long long> && std::is_same_v<decltype(m_height), long long>);
        if (fprintf(fp, "P6\n%lld %lld\n255\n", m_width, m_height) <= 0)
        {
                error("Error writing image header");
        }

        std::vector<std::array<unsigned char, 3>> buffer(m_data.size());

        for (size_t i = 0; i < m_data.size(); ++i)
        {
                buffer[i] = rgb_float_to_srgb_integer(m_data[i]);
        }

        if (fwrite(buffer.data(), 3, buffer.size(), fp) != buffer.size())
        {
                error("Error writing image data");
        }
}

// Текстурные координаты могут отсчитываться снизу, поэтому нужна эта функция
void Image::flip_vertically()
{
        std::vector<vec3> tmp = m_data;

        for (int y = 0; y < m_height; ++y)
        {
                for (int x = 0; x < m_width; ++x)
                {
                        int index_from = y * m_width + x;
                        int index_to = (m_height - y - 1) * m_width + x;
                        m_data[index_to] = tmp[index_from];
                }
        }
}
