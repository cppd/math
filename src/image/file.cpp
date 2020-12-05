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

void save_1(const std::string& file_name, const ImageView<2>& image_view)
{
        ASSERT(1 == format_component_count(image_view.color_format));

        int width = image_view.size[0];
        int height = image_view.size[1];

        QImage q_image(width, height, QImage::Format_RGB32);

        std::vector<std::byte> bytes;
        format_conversion(image_view.color_format, image_view.pixels, ColorFormat::R8_SRGB, &bytes);

        ASSERT(1ull * width * height == bytes.size());

        const std::byte* ptr = bytes.data();
        for (int row = 0; row < height; ++row)
        {
                QRgb* image_line = reinterpret_cast<QRgb*>(q_image.scanLine(row));
                for (int col = 0; col < width; ++col)
                {
                        std::uint8_t c;
                        std::memcpy(&c, ptr++, 1);
                        image_line[col] = qRgb(c, c, c);
                }
        }
        if (!q_image.save(QString::fromStdString(file_name)))
        {
                error("Error saving pixels to the file " + file_name);
        }
}

void save_3(const std::string& file_name, const ImageView<2>& image_view)
{
        ASSERT(3 == format_component_count(image_view.color_format));

        int width = image_view.size[0];
        int height = image_view.size[1];

        QImage q_image(width, height, QImage::Format_RGB32);

        std::vector<std::byte> bytes;
        format_conversion(image_view.color_format, image_view.pixels, ColorFormat::R8G8B8_SRGB, &bytes);

        ASSERT(3ull * width * height == bytes.size());

        const std::byte* ptr = bytes.data();
        for (int row = 0; row < height; ++row)
        {
                QRgb* image_line = reinterpret_cast<QRgb*>(q_image.scanLine(row));
                for (int col = 0; col < width; ++col)
                {
                        std::uint8_t r;
                        std::uint8_t g;
                        std::uint8_t b;
                        std::memcpy(&r, ptr++, 1);
                        std::memcpy(&g, ptr++, 1);
                        std::memcpy(&b, ptr++, 1);
                        image_line[col] = qRgb(r, g, b);
                }
        }
        if (!q_image.save(QString::fromStdString(file_name)))
        {
                error("Error saving pixels to the file " + file_name);
        }
}

void save_4(const std::string& file_name, const ImageView<2>& image_view)
{
        ASSERT(4 == format_component_count(image_view.color_format));

        int width = image_view.size[0];
        int height = image_view.size[1];

        QImage q_image(width, height, QImage::Format_ARGB32);

        std::vector<std::byte> bytes;
        format_conversion(image_view.color_format, image_view.pixels, ColorFormat::R8G8B8A8_SRGB, &bytes);

        ASSERT(4ull * width * height == bytes.size());

        const std::byte* ptr = bytes.data();
        for (int row = 0; row < height; ++row)
        {
                QRgb* image_line = reinterpret_cast<QRgb*>(q_image.scanLine(row));
                for (int col = 0; col < width; ++col)
                {
                        std::uint8_t r;
                        std::uint8_t g;
                        std::uint8_t b;
                        std::uint8_t a;
                        std::memcpy(&r, ptr++, 1);
                        std::memcpy(&g, ptr++, 1);
                        std::memcpy(&b, ptr++, 1);
                        std::memcpy(&a, ptr++, 1);
                        image_line[col] = qRgba(r, g, b, a);
                }
        }
        if (!q_image.save(QString::fromStdString(file_name)))
        {
                error("Error saving pixels to the file " + file_name);
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

void check_colors(const QImage& image, const std::string& file_name)
{
        if (image.format() == QImage::Format_Invalid || image.format() == QImage::Format_Alpha8
            || image.pixelFormat().colorModel() == QPixelFormat::Alpha)
        {
                error("Colors not found in the file " + file_name);
        }
}
}

std::array<int, 2> find_size(const std::filesystem::path& path)
{
        std::string file_name = generic_utf8_filename(path);
        QImage q_image;
        if (!q_image.load(QString::fromStdString(file_name)) || q_image.width() < 1 || q_image.height() < 1)
        {
                error("Error loading image from the file " + file_name);
        }
        return {q_image.width(), q_image.height()};
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

void load_rgba(const std::filesystem::path& path, const std::array<int, 2>& size, const std::span<std::byte>& pixels)
{
        std::string file_name = generic_utf8_filename(path);
        QImage q_image;
        if (!q_image.load(QString::fromStdString(file_name)))
        {
                error("Error loading image from the file " + file_name);
        }
        if (q_image.width() != size[0] || q_image.height() != size[1])
        {
                error("Expected image size (" + to_string(size[0]) + ", " + to_string(size[1]) + "), found size ("
                      + to_string(q_image.width()) + ", " + to_string(q_image.height()) + ") in the file " + file_name);
        }
        check_colors(q_image, file_name);

        load_4(std::move(q_image), pixels);
}

Image<2> load_rgba(const std::filesystem::path& path)
{
        std::string file_name = generic_utf8_filename(path);
        QImage q_image;
        if (!q_image.load(QString::fromStdString(file_name)))
        {
                error("Error loading image from the file " + file_name);
        }
        if (q_image.width() < 1 || q_image.height() < 1)
        {
                error("Error image size (" + to_string(q_image.width()) + ", " + to_string(q_image.height())
                      + ") in the file " + file_name);
        }
        check_colors(q_image, file_name);

        Image<2> image;
        image.color_format = ColorFormat::R8G8B8A8_SRGB;
        image.size[0] = q_image.width();
        image.size[1] = q_image.height();
        image.pixels.resize(4ull * image.size[0] * image.size[1]);

        load_4(std::move(q_image), image.pixels);

        return image;
}
}
