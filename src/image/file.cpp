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

#include <src/com/error.h>
#include <src/utility/file/sys.h>
#include <src/utility/string/str.h>

#include <QImage>
#include <QImageWriter>
#include <cstring>
#include <set>

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
}

void save_grayscale_image_to_file(
        const std::string& file_name,
        int width,
        int height,
        ColorFormat color_format,
        Span<const std::byte> pixels)
{
        if (color_format != ColorFormat::R8_SRGB)
        {
                error("Image format is not supported for saving grayscale image to file");
        }

        if (1ull * width * height != pixels.size())
        {
                error("Error image size");
        }

        QImage image(width, height, QImage::Format_RGB32);
        size_t pixel = 0;
        for (int row = 0; row < height; ++row)
        {
                QRgb* image_line = reinterpret_cast<QRgb*>(image.scanLine(row));
                for (int col = 0; col < width; ++col)
                {
                        std::uint8_t c;
                        std::memcpy(&c, &pixels[pixel++], 1);
                        image_line[col] = qRgb(c, c, c);
                }
        }

        std::string f = file_name_with_extension(file_name);
        if (!image.save(f.c_str()))
        {
                error("Error saving pixels to the file " + f);
        }
}

void save_srgb_image_to_file(
        const std::string& file_name,
        int width,
        int height,
        const std::vector<std::uint_least8_t>& pixels)
{
        if (3ull * width * height != pixels.size())
        {
                error("Error image size");
        }

        QImage image(width, height, QImage::Format_RGB32);
        size_t pixel = 0;
        for (int row = 0; row < height; ++row)
        {
                QRgb* image_line = reinterpret_cast<QRgb*>(image.scanLine(row));
                for (int col = 0; col < width; ++col)
                {
                        std::uint_least8_t r = pixels[pixel++];
                        std::uint_least8_t g = pixels[pixel++];
                        std::uint_least8_t b = pixels[pixel++];
                        image_line[col] = qRgb(r, g, b);
                }
        }

        std::string f = file_name_with_extension(file_name);
        if (!image.save(f.c_str()))
        {
                error("Error saving pixels to the file " + f);
        }
}

void save_srgb_image_to_file_bgr(
        const std::string& file_name,
        int width,
        int height,
        const std::vector<std::uint_least32_t>& pixels)
{
        if (1ull * width * height != pixels.size())
        {
                error("Error image size");
        }

        static_assert(std::is_same_v<quint32, std::uint_least32_t>);
        const size_t bytes_in_row = sizeof(std::uint_least32_t) * width;
        QImage image(width, height, QImage::Format_RGB32);
        const std::uint_least32_t* pixels_line = pixels.data();
        for (int row = 0; row < height; ++row, pixels_line += width)
        {
                QRgb* image_line = reinterpret_cast<QRgb*>(image.scanLine(row));
                std::memcpy(image_line, pixels_line, bytes_in_row);
        }

        std::string f = file_name_with_extension(file_name);
        if (!image.save(f.c_str()))
        {
                error("Error saving pixels to the file " + f);
        }
}

void load_image_from_file(
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
        if (color_format != ColorFormat::R8G8B8A8_SRGB)
        {
                error("Image format is not supported for image flipping");
        }

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
