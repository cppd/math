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
#include "com/string/str.h"

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
        for (QString s : QImageWriter::supportedImageFormats())
        {
                formats.insert(s.toLower().toStdString());
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

        if (ext.size() > 0)
        {
                checkWriteFormatSupport(ext);
                return file_name;
        }

        checkWriteFormatSupport(DEFAULT_WRITE_FORMAT);
        // Если имя заканчивается на точку, то пусть будет 2 точки подряд
        return file_name + "." + DEFAULT_WRITE_FORMAT;
}
}

void save_grayscale_image_to_file(const std::string& file_name, int width, int height, Span<const std::uint_least8_t> pixels)
{
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
                        std::uint_least8_t c = pixels[pixel++];
                        image_line[col] = qRgb(c, c, c);
                }
        }

        std::string f = file_name_with_extension(file_name);
        if (!image.save(f.c_str()))
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

void save_srgb_image_to_file_bgr(const std::string& file_name, int width, int height,
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

void load_srgba_image_from_file(const std::string& file_name, int* width, int* height, std::vector<std::uint_least8_t>* pixels)
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
                        (*pixels)[pixel++] = qRed(c);
                        (*pixels)[pixel++] = qGreen(c);
                        (*pixels)[pixel++] = qBlue(c);
                        (*pixels)[pixel++] = qAlpha(c);
                }
        }
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
