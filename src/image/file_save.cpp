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

#include "file_save.h"

#include "conversion.h"
#include "format.h"
#include "image.h"

#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/print.h>
#include <src/com/string/str.h>

#include <QImage>
#include <QImageWriter>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ns::image
{
namespace
{
constexpr std::string_view WRITE_FORMAT = "png";

std::set<std::string> supported_formats()
{
        std::set<std::string> res;
        for (const QByteArray& a : QImageWriter::supportedImageFormats())
        {
                res.insert(QString(a).toLower().toStdString());
        }
        return res;
}

void check_write_format_support(const std::string_view format)
{
        static const std::set<std::string> formats = supported_formats();

        if (formats.contains(to_lower(format)))
        {
                return;
        }

        std::string s;
        for (const std::string& f : formats)
        {
                if (!s.empty())
                {
                        s += ", ";
                }
                s += f;
        }

        error("Unsupported format \"" + std::string(format) + "\" for image writing, supported formats " + s);
}

std::filesystem::path file_name_with_extension(std::filesystem::path path)
{
        const std::string extension = generic_utf8_filename(path.extension());
        if (!extension.empty() && extension[0] == '.')
        {
                check_write_format_support(extension.substr(1));
                return path;
        }

        check_write_format_support(WRITE_FORMAT);
        return path.replace_extension(WRITE_FORMAT);
}

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

void save_image(
        const std::size_t width,
        const std::size_t height,
        const QImage::Format q_format,
        const ColorFormat color_format,
        const std::span<const std::byte> bytes,
        const std::string& file_name)
{
        check_size(width, height, color_format, bytes.size());

        QImage image(width, height, q_format);

        const std::size_t pixel_size = format_pixel_size_in_bytes(color_format);
        const std::size_t row_size = pixel_size * width;
        ASSERT(row_size <= static_cast<std::size_t>(image.bytesPerLine()));

        const std::byte* ptr = bytes.data();
        for (unsigned row = 0; row < height; ++row, ptr += row_size)
        {
                std::memcpy(image.scanLine(row), ptr, row_size);
        }

        if (!image.save(QString::fromStdString(file_name)))
        {
                error("Error saving pixels to the file " + file_name);
        }
}

void save_image_alpha(
        const std::size_t width,
        const std::size_t height,
        const QImage::Format q_format,
        const ColorFormat color_format,
        const std::span<const std::byte> bytes,
        const std::string& file_name)
{
        ASSERT(color_format == ColorFormat::R16G16B16_SRGB);

        check_size(width, height, color_format, bytes.size());

        constexpr std::uint16_t ALPHA = 65535;

        QImage image(width, height, q_format);

        const std::size_t pixel_size = format_pixel_size_in_bytes(color_format);
        ASSERT((pixel_size + sizeof(ALPHA)) * width <= static_cast<std::size_t>(image.bytesPerLine()));

        const std::byte* ptr = bytes.data();
        for (unsigned row = 0; row < height; ++row)
        {
                unsigned char* image_ptr = image.scanLine(row);
                for (unsigned col = 0; col < width; ++col)
                {
                        std::memcpy(image_ptr, ptr, pixel_size);
                        image_ptr += pixel_size;
                        std::memcpy(image_ptr, &ALPHA, sizeof(ALPHA));
                        image_ptr += sizeof(ALPHA);
                        ptr += pixel_size;
                }
        }

        if (!image.save(QString::fromStdString(file_name)))
        {
                error("Error saving pixels to the file " + file_name);
        }
}

void save(const std::string& file_name, const ImageView<2>& image_view)
{
        const std::size_t width = image_view.size[0];
        const std::size_t height = image_view.size[1];

        check_size(width, height, image_view.color_format, image_view.pixels.size());

        switch (image_view.color_format)
        {
        case ColorFormat::R8_SRGB:
        {
                save_image(
                        width, height, QImage::Format_Grayscale8, ColorFormat::R8_SRGB, image_view.pixels, file_name);
                return;
        }
        case ColorFormat::R16:
        {
                save_image(width, height, QImage::Format_Grayscale16, ColorFormat::R16, image_view.pixels, file_name);
                return;
        }
        case ColorFormat::R32:
        {
                std::vector<std::byte> bytes;
                format_conversion(image_view.color_format, image_view.pixels, ColorFormat::R16, &bytes);
                save_image(width, height, QImage::Format_Grayscale16, ColorFormat::R16, bytes, file_name);
                return;
        }
        case ColorFormat::R8G8B8_SRGB:
        {
                save_image(
                        width, height, QImage::Format_RGB888, ColorFormat::R8G8B8_SRGB, image_view.pixels, file_name);
                return;
        }
        case ColorFormat::R16G16B16:
        {
                std::vector<std::byte> bytes;
                format_conversion(image_view.color_format, image_view.pixels, ColorFormat::R16G16B16_SRGB, &bytes);
                save_image_alpha(width, height, QImage::Format_RGBX64, ColorFormat::R16G16B16_SRGB, bytes, file_name);
                return;
        }
        case ColorFormat::R16G16B16_SRGB:
        {
                save_image_alpha(
                        width, height, QImage::Format_RGBX64, ColorFormat::R16G16B16_SRGB, image_view.pixels,
                        file_name);
                return;
        }
        case ColorFormat::R32G32B32:
        {
                std::vector<std::byte> bytes;
                format_conversion(image_view.color_format, image_view.pixels, ColorFormat::R16G16B16_SRGB, &bytes);
                save_image_alpha(width, height, QImage::Format_RGBX64, ColorFormat::R16G16B16_SRGB, bytes, file_name);
                return;
        }
        case ColorFormat::R8G8B8A8_SRGB:
        {
                save_image(
                        width, height, QImage::Format_RGBA8888, ColorFormat::R8G8B8A8_SRGB, image_view.pixels,
                        file_name);
                return;
        }
        case ColorFormat::R16G16B16A16:
        {
                std::vector<std::byte> bytes;
                format_conversion(image_view.color_format, image_view.pixels, ColorFormat::R16G16B16A16_SRGB, &bytes);
                save_image(width, height, QImage::Format_RGBA64, ColorFormat::R16G16B16A16_SRGB, bytes, file_name);
                return;
        }
        case ColorFormat::R16G16B16A16_SRGB:
        {
                save_image(
                        width, height, QImage::Format_RGBA64, ColorFormat::R16G16B16A16_SRGB, image_view.pixels,
                        file_name);
                return;
        }
        case ColorFormat::R32G32B32A32:
        {
                std::vector<std::byte> bytes;
                format_conversion(image_view.color_format, image_view.pixels, ColorFormat::R16G16B16A16_SRGB, &bytes);
                save_image(width, height, QImage::Format_RGBA64, ColorFormat::R16G16B16A16_SRGB, bytes, file_name);
                return;
        }
        case ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
        case ColorFormat::R16G16B16A16_PREMULTIPLIED:
        case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                error("Premultiplied image formats are not supported for saving image to file");
        }
        error("Unknown format " + format_to_string(image_view.color_format) + " for saving image");
}
}

std::string_view save_file_extension()
{
        return WRITE_FORMAT;
}

template <typename Path>
void save(const Path& path, const ImageView<2>& image_view)
{
        static_assert(std::is_same_v<Path, std::filesystem::path>);

        save(generic_utf8_filename(file_name_with_extension(path)), image_view);
}

template void save(const std::filesystem::path&, const ImageView<2>&);
}
