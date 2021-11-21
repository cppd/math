/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "conversion.h"

#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/print.h>
#include <src/com/string/str.h>

#include <QColorSpace>
#include <QImage>
#include <QImageWriter>
#include <cstring>
#include <filesystem>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace ns::image
{
namespace
{
constexpr std::string_view WRITE_FORMAT = "png";

std::set<std::string> supported_formats()
{
        std::set<std::string> formats;
        for (const QByteArray& a : QImageWriter::supportedImageFormats())
        {
                formats.insert(QString(a).toLower().toStdString());
        }
        return formats;
}

void check_write_format_support(const std::string_view& format)
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
        std::string extension = generic_utf8_filename(path.extension());
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
        const std::span<const std::byte>& bytes,
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
        const std::span<const std::byte>& bytes,
        const std::string& file_name)
{
        ASSERT(color_format == ColorFormat::R16G16B16_SRGB);

        check_size(width, height, color_format, bytes.size());

        constexpr uint16_t ALPHA = 65535;

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

void load_image(QImage* const image, const ColorFormat color_format, const std::span<std::byte>& bytes)
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

void load_image_alpha(QImage* const image, const ColorFormat color_format, const std::span<std::byte>& bytes)
{
        ASSERT(color_format == ColorFormat::R16G16B16_SRGB);

        check_size(image->width(), image->height(), color_format, bytes.size());

        const std::size_t pixel_size = format_pixel_size_in_bytes(color_format);
        const std::size_t image_pixel_size = pixel_size + sizeof(uint16_t);
        ASSERT(image_pixel_size * image->width() <= static_cast<std::size_t>(image->bytesPerLine()));

        int width = image->width();
        int height = image->height();

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

void load_1(QImage&& image, const ColorFormat color_format, const std::span<std::byte>& bytes)
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

void load_3(QImage&& image, const ColorFormat color_format, const std::span<std::byte>& bytes)
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

void load_4(QImage&& image, const ColorFormat color_format, const std::span<std::byte>& bytes)
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

template <typename T>
QImage open_image(const T& path)
{
        static_assert(std::is_same_v<T, std::filesystem::path>);

        std::string file_name = generic_utf8_filename(path);
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
        std::unordered_map<ColorFormat, std::unordered_set<QImage::Format>> map;

        for (const auto& [q_format, color_format] : q_image_format_to_color_format_map())
        {
                map[color_format].insert(q_format);
        }

        return map;
}

ColorFormat q_format_to_color_format(const QImage::Format format)
{
        static const std::unordered_map<QImage::Format, ColorFormat> map = q_image_format_to_color_format_map();
        auto iter = map.find(format);
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
        auto iter = map.find(format);
        if (iter != map.cend())
        {
                return iter->second;
        }
        error("Error finding QImage format: unsupported color format " + format_to_string(format));
}
}

std::string_view file_extension()
{
        return WRITE_FORMAT;
}

template <typename Path>
void save(const Path& path, const ImageView<2>& image_view)
{
        static_assert(std::is_same_v<Path, std::filesystem::path>);

        std::string file_name = generic_utf8_filename(file_name_with_extension(path));

        save(file_name, image_view);
}

template <typename Path>
Info file_info(const Path& path)
{
        static_assert(std::is_same_v<Path, std::filesystem::path>);

        QImage image = open_image(path);
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
        const std::span<std::byte>& pixels)
{
        static_assert(std::is_same_v<Path, std::filesystem::path>);

        check_size(size[0], size[1], color_format, pixels.size());

        QImage image = open_image(path);

        if (image.width() != size[0] || image.height() != size[1])
        {
                std::string expected = "(" + to_string(size[0]) + ", " + to_string(size[1]) + ")";
                std::string found = "(" + to_string(image.width()) + ", " + to_string(image.height()) + ")";
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

        QImage q_image = open_image(path);

        Image<2> image;
        image.color_format = ColorFormat::R8G8B8A8_SRGB;
        image.size[0] = q_image.width();
        image.size[1] = q_image.height();
        const std::size_t pixel_size = format_pixel_size_in_bytes(image.color_format);
        image.pixels.resize(pixel_size * image.size[0] * image.size[1]);

        load_4(std::move(q_image), image.color_format, image.pixels);

        return image;
}

template void save(const std::filesystem::path&, const ImageView<2>&);
template Info file_info(const std::filesystem::path&);
template Image<2> load_rgba(const std::filesystem::path&);
template void load(const std::filesystem::path&, ColorFormat, const std::array<int, 2>&, const std::span<std::byte>&);
}
