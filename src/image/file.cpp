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

#include "file.h"

#include "conversion.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/utility/file/path.h>
#include <src/utility/string/str.h>

#include <QColorSpace>
#include <QImage>
#include <QImageWriter>
#include <cstring>
#include <set>

namespace image
{
namespace
{
constexpr const char* DEFAULT_WRITE_FORMAT = "png";

std::set<std::string> supported_formats()
{
        std::set<std::string> formats;
        for (const QByteArray& a : QImageWriter::supportedImageFormats())
        {
                formats.insert(QString(a).toLower().toStdString());
        }
        return formats;
}

void check_write_format_support(const std::string& format)
{
        const static std::set<std::string> formats = supported_formats();

        if (formats.count(to_lower(format)) != 0)
        {
                return;
        }

        std::string format_string;
        for (const std::string& f : formats)
        {
                if (!format_string.empty())
                {
                        format_string += ", ";
                }
                format_string += f;
        }
        error("Unsupported format for image writing \"" + format + "\", supported formats " + format_string);
}

std::filesystem::path file_name_with_extension(std::filesystem::path path)
{
        std::string extension = generic_utf8_filename(path.extension());
        if (!extension.empty() && extension[0] == '.')
        {
                check_write_format_support(extension.substr(1));
                return path;
        }

        check_write_format_support(DEFAULT_WRITE_FORMAT);
        return path.replace_extension(DEFAULT_WRITE_FORMAT);
}

template <typename T>
std::vector<std::byte> add_alpha(const std::span<const std::byte>& bytes3)
{
        std::vector<std::byte> bytes;
        bytes.resize((bytes3.size() / 3) * 4);
        ASSERT((bytes.size() / 4) * 3 == bytes3.size());

        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);
        const T alpha = std::is_integral_v<T> ? limits<T>::max() : T(1);
        const size_t pixel_count = bytes3.size() / sizeof(T) / 3;
        ASSERT(pixel_count * sizeof(T) * 3 == bytes3.size());

        const std::byte* src = bytes3.data();
        std::byte* dst = bytes.data();
        for (size_t i = 0; i < pixel_count; ++i)
        {
                std::memcpy(dst, src, 3 * sizeof(T));
                dst += 3 * sizeof(T);
                std::memcpy(dst, &alpha, sizeof(T));
                dst += sizeof(T);
                src += 3 * sizeof(T);
        }
        return bytes;
}

std::vector<std::byte> add_alpha(ColorFormat color_format, const std::span<const std::byte>& bytes)
{
        if (format_component_count(color_format) != 3)
        {
                error("Error image format " + format_to_string(color_format) + " for adding alpha value");
        }
        if (color_format == ColorFormat::R16G16B16)
        {
                return add_alpha<uint16_t>(bytes);
        }
        if (color_format == ColorFormat::R32G32B32)
        {
                return add_alpha<float>(bytes);
        }
        if (color_format == ColorFormat::R8G8B8_SRGB)
        {
                return add_alpha<uint8_t>(bytes);
        }
        error("Unsupported image format " + format_to_string(color_format) + " for adding alpha value");
}

void save_image(
        size_t width,
        size_t height,
        QImage::Format q_format,
        ColorFormat format,
        const std::span<const std::byte>& bytes,
        const std::string& file_name)
{
        QImage q_image(width, height, q_format);

        const size_t row_size = format_pixel_size_in_bytes(format) * width;
        ASSERT(row_size * height == bytes.size());
        ASSERT(row_size <= static_cast<size_t>(q_image.bytesPerLine()));

        const std::byte* ptr = bytes.data();
        for (unsigned row = 0; row < height; ++row, ptr += row_size)
        {
                std::memcpy(q_image.scanLine(row), ptr, row_size);
        }

        if (!q_image.save(QString::fromStdString(file_name)))
        {
                error("Error saving pixels to the file " + file_name);
        }
}

void save_1(const std::string& file_name, const ImageView<2>& image_view)
{
        ASSERT(1 == format_component_count(image_view.color_format));

        size_t width = image_view.size[0];
        size_t height = image_view.size[1];

        if (image_view.color_format == ColorFormat::R8_SRGB)
        {
                save_image(
                        width, height, QImage::Format_Grayscale8, ColorFormat::R8_SRGB, image_view.pixels, file_name);
        }
        else if (image_view.color_format == ColorFormat::R16)
        {
                save_image(width, height, QImage::Format_Grayscale16, ColorFormat::R16, image_view.pixels, file_name);
        }
        else if (image_view.color_format == ColorFormat::R32)
        {
                std::vector<std::byte> bytes;
                format_conversion(image_view.color_format, image_view.pixels, ColorFormat::R16, &bytes);
                save_image(width, height, QImage::Format_Grayscale16, ColorFormat::R16, image_view.pixels, file_name);
        }
        else
        {
                error("Unsupported format " + format_to_string(image_view.color_format) + "for saving grayscale image");
        }
}

void save_3(const std::string& file_name, const ImageView<2>& image_view)
{
        ASSERT(3 == format_component_count(image_view.color_format));

        size_t width = image_view.size[0];
        size_t height = image_view.size[1];

        if (image_view.color_format == ColorFormat::R8G8B8_SRGB)
        {
                save_image(
                        width, height, QImage::Format_RGB888, ColorFormat::R8G8B8_SRGB, image_view.pixels, file_name);
        }
        else if (image_view.color_format == ColorFormat::R16G16B16)
        {
                save_image(
                        width, height, QImage::Format_RGBX64, ColorFormat::R16G16B16A16,
                        add_alpha(ColorFormat::R16G16B16, image_view.pixels), file_name);
        }
        else if (image_view.color_format == ColorFormat::R32G32B32)
        {
                std::vector<std::byte> bytes;
                format_conversion(image_view.color_format, image_view.pixels, ColorFormat::R16G16B16, &bytes);
                save_image(
                        width, height, QImage::Format_RGBX64, ColorFormat::R16G16B16A16,
                        add_alpha(ColorFormat::R16G16B16, bytes), file_name);
        }
        else
        {
                error("Unsupported format " + format_to_string(image_view.color_format) + "for saving RGB image");
        }
}

void save_4(const std::string& file_name, const ImageView<2>& image_view)
{
        ASSERT(4 == format_component_count(image_view.color_format));

        size_t width = image_view.size[0];
        size_t height = image_view.size[1];

        if (image_view.color_format == ColorFormat::R8G8B8A8_SRGB)
        {
                save_image(
                        width, height, QImage::Format_RGBA8888, ColorFormat::R8G8B8A8_SRGB, image_view.pixels,
                        file_name);
        }
        else if (image_view.color_format == ColorFormat::R16G16B16A16)
        {
                save_image(
                        width, height, QImage::Format_RGBA64, ColorFormat::R16G16B16A16, image_view.pixels, file_name);
        }
        else if (image_view.color_format == ColorFormat::R32G32B32A32)
        {
                std::vector<std::byte> bytes;
                format_conversion(image_view.color_format, image_view.pixels, ColorFormat::R16G16B16A16, &bytes);
                save_image(width, height, QImage::Format_RGBX64, ColorFormat::R16G16B16A16, bytes, file_name);
        }
        else
        {
                error("Unsupported format " + format_to_string(image_view.color_format) + "for saving RGB image");
        }
}

void load_4(QImage&& image, const std::span<std::byte>& pixels)
{
        ASSERT(pixels.size() == 4ull * image.width() * image.height());

        if (image.format() != QImage::Format_ARGB32)
        {
                image = image.convertToFormat(QImage::Format_ARGB32);
        }

        int width = image.width();
        int height = image.height();

        std::byte* ptr = pixels.data();
        for (int row = 0; row < height; ++row)
        {
                const QRgb* image_line = reinterpret_cast<const QRgb*>(image.constScanLine(row));
                for (int col = 0; col < width; ++col, ptr += 4)
                {
                        const QRgb& c = image_line[col];
                        std::array<uint8_t, 4> rgba;
                        rgba[0] = qRed(c);
                        rgba[1] = qGreen(c);
                        rgba[2] = qBlue(c);
                        rgba[3] = qAlpha(c);
                        std::memcpy(ptr, rgba.data(), 4);
                }
        }
}

template <typename T>
std::enable_if_t<std::is_same_v<T, std::filesystem::path>, QImage> open_image(const T& path)
{
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
}

void save(const std::filesystem::path& path, const ImageView<2>& image_view)
{
        std::string file_name = generic_utf8_filename(file_name_with_extension(path));

        int width = image_view.size[0];
        int height = image_view.size[1];

        if (image_view.pixels.size() != 1ull * width * height * format_pixel_size_in_bytes(image_view.color_format))
        {
                error("Error image data size");
        }

        if (format_component_count(image_view.color_format) == 1)
        {
                save_1(file_name, image_view);
        }
        else if (format_component_count(image_view.color_format) == 3)
        {
                save_3(file_name, image_view);
        }
        else if (format_component_count(image_view.color_format) == 4)
        {
                save_4(file_name, image_view);
        }
        else
        {
                error("Image format " + format_to_string(image_view.color_format)
                      + " is not supported for saving to file");
        }
}

std::array<int, 2> find_size(const std::filesystem::path& path)
{
        QImage q_image = open_image(path);
        return {q_image.width(), q_image.height()};
}

void load_rgba(const std::filesystem::path& path, const std::array<int, 2>& size, const std::span<std::byte>& pixels)
{
        QImage q_image = open_image(path);

        if (q_image.width() != size[0] || q_image.height() != size[1])
        {
                std::string expected = "(" + to_string(size[0]) + ", " + to_string(size[1]) + ")";
                std::string found = "(" + to_string(q_image.width()) + ", " + to_string(q_image.height()) + ")";
                error("Expected image size " + expected + ", found size " + found + " in the file "
                      + generic_utf8_filename(path));
        }

        load_4(std::move(q_image), pixels);
}

Image<2> load_rgba(const std::filesystem::path& path)
{
        QImage q_image = open_image(path);

        Image<2> image;
        image.color_format = ColorFormat::R8G8B8A8_SRGB;
        image.size[0] = q_image.width();
        image.size[1] = q_image.height();
        image.pixels.resize(4ull * image.size[0] * image.size[1]);

        load_4(std::move(q_image), image.pixels);

        return image;
}
}
