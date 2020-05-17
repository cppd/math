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
#include <src/utility/file/sys.h>
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

void checkWriteFormatSupport(const std::string& format)
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
                        format_string += ", ";
                format_string += f;
        }
        error("Unsupported format for image writing \"" + format + "\", supported formats " + format_string);
}

std::string file_name_with_extension(const std::string& file_name)
{
        std::string ext = file_extension(file_name);

        if (!ext.empty())
        {
                checkWriteFormatSupport(ext);
                return file_name;
        }

        checkWriteFormatSupport(DEFAULT_WRITE_FORMAT);
        // Если имя заканчивается на точку, то пусть будет 2 точки подряд
        return file_name + "." + DEFAULT_WRITE_FORMAT;
}

void save_1(const std::string& file_name, int width, int height, ColorFormat color_format, Span<const std::byte> pixels)
{
        QImage image(width, height, QImage::Format_RGB32);

        std::vector<std::byte> bytes;
        format_conversion(color_format, pixels, ColorFormat::R8_SRGB, &bytes);

        ASSERT(1ull * width * height == pixels.size());

        const std::byte* ptr = bytes.data();
        for (int row = 0; row < height; ++row)
        {
                QRgb* image_line = reinterpret_cast<QRgb*>(image.scanLine(row));
                for (int col = 0; col < width; ++col)
                {
                        std::uint8_t c;
                        std::memcpy(&c, ptr++, 1);
                        image_line[col] = qRgb(c, c, c);
                }
        }
        if (!image.save(file_name.c_str()))
        {
                error("Error saving pixels to the file " + file_name);
        }
}

void save_3(const std::string& file_name, int width, int height, ColorFormat color_format, Span<const std::byte> pixels)
{
        QImage image(width, height, QImage::Format_RGB32);

        std::vector<std::byte> bytes;
        format_conversion(color_format, pixels, ColorFormat::R8G8B8_SRGB, &bytes);

        ASSERT(3ull * width * height == pixels.size());

        const std::byte* ptr = pixels.data();
        for (int row = 0; row < height; ++row)
        {
                QRgb* image_line = reinterpret_cast<QRgb*>(image.scanLine(row));
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
        if (!image.save(file_name.c_str()))
        {
                error("Error saving pixels to the file " + file_name);
        }
}

void save_4(const std::string& file_name, int width, int height, ColorFormat color_format, Span<const std::byte> pixels)
{
        QImage image(width, height, QImage::Format_ARGB32);

        std::vector<std::byte> bytes;
        format_conversion(color_format, pixels, ColorFormat::R8G8B8A8_SRGB, &bytes);

        ASSERT(4ull * width * height == pixels.size());

        const std::byte* ptr = pixels.data();
        for (int row = 0; row < height; ++row)
        {
                QRgb* image_line = reinterpret_cast<QRgb*>(image.scanLine(row));
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
        if (!image.save(file_name.c_str()))
        {
                error("Error saving pixels to the file " + file_name);
        }
}
}

void save_image_to_file(
        const std::string& file_name,
        int width,
        int height,
        ColorFormat color_format,
        Span<const std::byte> pixels)
{
        std::string f = file_name_with_extension(file_name);

        if (pixels.size() != 1ull * width * height * pixel_size_in_bytes(color_format))
        {
                error("Error image data size");
        }

        if (component_count(color_format) == 1)
        {
                save_1(f, width, height, color_format, pixels);
        }
        else if (component_count(color_format) == 3)
        {
                save_3(f, width, height, color_format, pixels);
        }
        else if (component_count(color_format) == 4)
        {
                save_4(f, width, height, color_format, pixels);
        }
        else
        {
                error("Image format " + format_to_string(color_format) + " is not supported for saving to file");
        }
}

void load_image_from_file_rgb(
        const std::string& file_name,
        int* width,
        int* height,
        ColorFormat* color_format,
        std::vector<std::byte>* pixels)
{
        QImage image;
        if (!image.load(file_name.c_str()) || image.width() < 1 || image.height() < 1)
        {
                error("Error loading image from the file " + file_name);
        }

        if (image.format() != QImage::Format_ARGB32)
        {
                image = image.convertToFormat(QImage::Format_ARGB32);
        }

        *width = image.width();
        *height = image.height();

        pixels->resize(3ull * (*width) * (*height));

        size_t pixel = 0;
        for (int row = 0; row < *height; ++row)
        {
                const QRgb* image_line = reinterpret_cast<const QRgb*>(image.constScanLine(row));
                for (int col = 0; col < *width; ++col)
                {
                        const QRgb& c = image_line[col];
                        uint8_t r = qRed(c);
                        uint8_t g = qGreen(c);
                        uint8_t b = qBlue(c);
                        std::memcpy(&(*pixels)[pixel++], &r, 1);
                        std::memcpy(&(*pixels)[pixel++], &g, 1);
                        std::memcpy(&(*pixels)[pixel++], &b, 1);
                }
        }

        *color_format = ColorFormat::R8G8B8_SRGB;
}

void load_image_from_file_rgba(
        const std::string& file_name,
        int* width,
        int* height,
        ColorFormat* color_format,
        std::vector<std::byte>* pixels)
{
        QImage image;
        if (!image.load(file_name.c_str()) || image.width() < 1 || image.height() < 1)
        {
                error("Error loading image from the file " + file_name);
        }

        if (image.format() != QImage::Format_ARGB32)
        {
                image = image.convertToFormat(QImage::Format_ARGB32);
        }

        *width = image.width();
        *height = image.height();

        pixels->resize(4ull * (*width) * (*height));

        size_t pixel = 0;
        for (int row = 0; row < *height; ++row)
        {
                const QRgb* image_line = reinterpret_cast<const QRgb*>(image.constScanLine(row));
                for (int col = 0; col < *width; ++col)
                {
                        const QRgb& c = image_line[col];
                        uint8_t r = qRed(c);
                        uint8_t g = qGreen(c);
                        uint8_t b = qBlue(c);
                        uint8_t a = qAlpha(c);
                        std::memcpy(&(*pixels)[pixel++], &r, 1);
                        std::memcpy(&(*pixels)[pixel++], &g, 1);
                        std::memcpy(&(*pixels)[pixel++], &b, 1);
                        std::memcpy(&(*pixels)[pixel++], &a, 1);
                }
        }

        *color_format = ColorFormat::R8G8B8A8_SRGB;
}

void flip_image_vertically(int width, int height, ColorFormat color_format, std::vector<std::byte>* pixels)
{
        size_t pixel_size = pixel_size_in_bytes(color_format);

        if (pixels->size() != pixel_size * width * height)
        {
                error("Error image size");
        }

        size_t row_size = pixel_size * width;
        size_t row_end = (height / 2) * row_size;
        for (size_t row1 = 0, row2 = (height - 1) * row_size; row1 < row_end; row1 += row_size, row2 -= row_size)
        {
                for (size_t i = 0; i < row_size; ++i)
                {
                        std::swap((*pixels)[row1 + i], (*pixels)[row2 + i]);
                }
        }
}
}
