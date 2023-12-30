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

#include "file_load.h"

#include "format.h"
#include "image.h"

#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/print.h>

#include <QImage>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace ns::image
{
namespace
{
void check_size(
        const std::size_t width,
        const std::size_t height,
        const ColorFormat format,
        const std::size_t byte_count)
{
        if (byte_count != format_pixel_size_in_bytes(format) * width * height)
        {
                error("Error data size " + to_string(byte_count) + " for image size (" + to_string(width) + ", "
                      + to_string(height) + ") and format " + format_to_string(format));
        }
}

void load_image(QImage* const image, const ColorFormat color_format, const std::span<std::byte> bytes)
{
        check_size(image->width(), image->height(), color_format, bytes.size());

        const std::size_t pixel_size = format_pixel_size_in_bytes(color_format);
        const std::size_t row_size = pixel_size * image->width();
        ASSERT(row_size <= static_cast<std::size_t>(image->bytesPerLine()));

        std::byte* ptr = bytes.data();
        for (int row = 0, height = image->height(); row < height; ++row, ptr += row_size)
        {
                std::memcpy(ptr, image->scanLine(row), row_size);
        }
}

void load_image_alpha(QImage* const image, const ColorFormat color_format, const std::span<std::byte> bytes)
{
        ASSERT(color_format == ColorFormat::R16G16B16_SRGB);

        check_size(image->width(), image->height(), color_format, bytes.size());

        const std::size_t pixel_size = format_pixel_size_in_bytes(color_format);
        const std::size_t image_pixel_size = pixel_size + sizeof(std::uint16_t);
        ASSERT(image_pixel_size * image->width() <= static_cast<std::size_t>(image->bytesPerLine()));

        const int width = image->width();
        const int height = image->height();

        std::byte* ptr = bytes.data();
        for (int row = 0; row < height; ++row)
        {
                const unsigned char* image_ptr = image->constScanLine(row);
                for (int col = 0; col < width; ++col)
                {
                        std::memcpy(ptr, image_ptr, pixel_size);
                        image_ptr += image_pixel_size;
                        ptr += pixel_size;
                }
        }
}

void load_1(QImage&& image, const ColorFormat color_format, const std::span<std::byte> bytes)
{
        check_size(image.width(), image.height(), color_format, bytes.size());

        if (color_format == ColorFormat::R8_SRGB)
        {
                if (image.format() != QImage::Format_Grayscale8)
                {
                        image = image.convertToFormat(QImage::Format_Grayscale8);
                }
                load_image(&image, color_format, bytes);
        }
        else if (color_format == ColorFormat::R16)
        {
                if (image.format() != QImage::Format_Grayscale16)
                {
                        image = image.convertToFormat(QImage::Format_Grayscale16);
                }
                load_image(&image, color_format, bytes);
        }
        else
        {
                error("Unsupported format " + format_to_string(color_format) + "for loading grayscale image");
        }
}

void load_3(QImage&& image, const ColorFormat color_format, const std::span<std::byte> bytes)
{
        check_size(image.width(), image.height(), color_format, bytes.size());

        if (color_format == ColorFormat::R8G8B8_SRGB)
        {
                if (image.format() != QImage::Format_RGB888)
                {
                        image = image.convertToFormat(QImage::Format_RGB888);
                }
                load_image(&image, color_format, bytes);
        }
        else if (color_format == ColorFormat::R16G16B16_SRGB)
        {
                if (image.format() != QImage::Format_RGBX64)
                {
                        image = image.convertToFormat(QImage::Format_RGBX64);
                }
                load_image_alpha(&image, color_format, bytes);
        }
        else
        {
                error("Unsupported format " + format_to_string(color_format) + "for loading RGB image");
        }
}

void load_4(QImage&& image, const ColorFormat color_format, const std::span<std::byte> bytes)
{
        check_size(image.width(), image.height(), color_format, bytes.size());

        if (color_format == ColorFormat::R8G8B8A8_SRGB)
        {
                if (image.format() != QImage::Format_RGBA8888)
                {
                        image = image.convertToFormat(QImage::Format_RGBA8888);
                }
                load_image(&image, color_format, bytes);
        }
        else if (color_format == ColorFormat::R16G16B16A16_SRGB)
        {
                if (image.format() != QImage::Format_RGBA64)
                {
                        image = image.convertToFormat(QImage::Format_RGBA64);
                }
                load_image(&image, color_format, bytes);
        }
        else
        {
                error("Unsupported format " + format_to_string(color_format) + "for loading RGBA image");
        }
}

QImage open_image(const std::string& file_name)
{
        QImage image;
        if (!image.load(QString::fromStdString(file_name)))
        {
                error("Error loading image from the file " + file_name);
        }
        if (image.width() < 1 || image.height() < 1)
        {
                error("Error image size (" + to_string(image.width()) + ", " + to_string(image.height())
                      + ") in the file " + file_name);
        }
        if (image.format() == QImage::Format_Invalid || image.format() == QImage::Format_Alpha8
            || image.pixelFormat().colorModel() == QPixelFormat::Alpha)
        {
                error("Colors not found in the file " + file_name);
        }
        return image;
}

std::unordered_map<QImage::Format, ColorFormat> q_image_format_to_color_format_map()
{
        std::unordered_map<QImage::Format, ColorFormat> map;

        map[QImage::Format_A2BGR30_Premultiplied] = ColorFormat::R8G8B8A8_SRGB;
        map[QImage::Format_A2RGB30_Premultiplied] = ColorFormat::R8G8B8A8_SRGB;
        map[QImage::Format_ARGB32] = ColorFormat::R8G8B8A8_SRGB;
        map[QImage::Format_ARGB32_Premultiplied] = ColorFormat::R8G8B8A8_SRGB;
        map[QImage::Format_ARGB4444_Premultiplied] = ColorFormat::R8G8B8A8_SRGB;
        map[QImage::Format_ARGB6666_Premultiplied] = ColorFormat::R8G8B8A8_SRGB;
        map[QImage::Format_ARGB8555_Premultiplied] = ColorFormat::R8G8B8A8_SRGB;
        map[QImage::Format_ARGB8565_Premultiplied] = ColorFormat::R8G8B8A8_SRGB;
        map[QImage::Format_BGR30] = ColorFormat::R8G8B8_SRGB;
        map[QImage::Format_BGR888] = ColorFormat::R8G8B8_SRGB;
        map[QImage::Format_Grayscale16] = ColorFormat::R16;
        map[QImage::Format_Grayscale8] = ColorFormat::R8_SRGB;
        map[QImage::Format_Indexed8] = ColorFormat::R8G8B8A8_SRGB;
        map[QImage::Format_MonoLSB] = ColorFormat::R8G8B8_SRGB;
        map[QImage::Format_Mono] = ColorFormat::R8G8B8_SRGB;
        map[QImage::Format_RGB16] = ColorFormat::R8G8B8_SRGB;
        map[QImage::Format_RGB30] = ColorFormat::R8G8B8_SRGB;
        map[QImage::Format_RGB32] = ColorFormat::R8G8B8_SRGB;
        map[QImage::Format_RGB444] = ColorFormat::R8G8B8_SRGB;
        map[QImage::Format_RGB555] = ColorFormat::R8G8B8_SRGB;
        map[QImage::Format_RGB666] = ColorFormat::R8G8B8_SRGB;
        map[QImage::Format_RGB888] = ColorFormat::R8G8B8_SRGB;
        map[QImage::Format_RGBA64] = ColorFormat::R16G16B16A16_SRGB;
        map[QImage::Format_RGBA64_Premultiplied] = ColorFormat::R16G16B16A16_SRGB;
        map[QImage::Format_RGBA8888] = ColorFormat::R8G8B8A8_SRGB;
        map[QImage::Format_RGBA8888_Premultiplied] = ColorFormat::R8G8B8A8_SRGB;
        map[QImage::Format_RGBX64] = ColorFormat::R16G16B16_SRGB;
        map[QImage::Format_RGBX8888] = ColorFormat::R8G8B8_SRGB;

        return map;
}

std::unordered_map<ColorFormat, std::unordered_set<QImage::Format>> color_format_to_q_image_format_map()
{
        std::unordered_map<ColorFormat, std::unordered_set<QImage::Format>> res;

        for (const auto& [q_format, color_format] : q_image_format_to_color_format_map())
        {
                res[color_format].insert(q_format);
        }

        return res;
}

ColorFormat q_format_to_color_format(const QImage::Format format)
{
        static const std::unordered_map<QImage::Format, ColorFormat> map = q_image_format_to_color_format_map();
        const auto iter = map.find(format);
        if (iter != map.cend())
        {
                return iter->second;
        }
        error("Error finding color format: unsupported QImage format " + to_string(enum_to_int(format)));
}

const std::unordered_set<QImage::Format>& color_format_to_q_format(const ColorFormat format)
{
        static const std::unordered_map<ColorFormat, std::unordered_set<QImage::Format>> map =
                color_format_to_q_image_format_map();
        const auto iter = map.find(format);
        if (iter != map.cend())
        {
                return iter->second;
        }
        error("Error finding QImage format: unsupported color format " + format_to_string(format));
}
}

template <typename Path>
Info file_info(const Path& path)
{
        static_assert(std::is_same_v<Path, std::filesystem::path>);

        const QImage image = open_image(path);

        Info info;
        info.size[0] = image.width();
        info.size[1] = image.height();
        info.format = q_format_to_color_format(image.format());
        return info;
}

template <typename Path>
void load(
        const Path& path,
        const ColorFormat color_format,
        const std::array<int, 2>& size,
        const std::span<std::byte> pixels)
{
        static_assert(std::is_same_v<Path, std::filesystem::path>);

        check_size(size[0], size[1], color_format, pixels.size());

        QImage image = open_image(path);

        if (image.width() != size[0] || image.height() != size[1])
        {
                const std::string expected = '(' + to_string(size[0]) + ", " + to_string(size[1]) + ')';
                const std::string found = '(' + to_string(image.width()) + ", " + to_string(image.height()) + ')';
                error("Expected image size " + expected + ", found size " + found + " in the file "
                      + generic_utf8_filename(path));
        }

        if (!color_format_to_q_format(color_format).contains(image.format()))
        {
                error("Wrong QImage format " + to_string(enum_to_int(image.format())) + " for color format "
                      + format_to_string(color_format));
        }

        switch (format_component_count(color_format))
        {
        case 1:
                load_1(std::move(image), color_format, pixels);
                break;
        case 3:
                load_3(std::move(image), color_format, pixels);
                break;
        case 4:
                load_4(std::move(image), color_format, pixels);
                break;
        default:
                error("Color format " + format_to_string(color_format) + " is not supported for loading from file");
        }
}

template <typename Path>
Image<2> load_rgba(const Path& path)
{
        static_assert(std::is_same_v<Path, std::filesystem::path>);

        QImage q_image = open_image(generic_utf8_filename(path));

        Image<2> image;
        image.color_format = ColorFormat::R8G8B8A8_SRGB;
        image.size[0] = q_image.width();
        image.size[1] = q_image.height();
        const std::size_t pixel_size = format_pixel_size_in_bytes(image.color_format);
        image.pixels.resize(pixel_size * image.size[0] * image.size[1]);

        load_4(std::move(q_image), image.color_format, image.pixels);

        return image;
}

template Info file_info(const std::filesystem::path&);
template Image<2> load_rgba(const std::filesystem::path&);
template void load(const std::filesystem::path&, ColorFormat, const std::array<int, 2>&, std::span<std::byte>);
}
