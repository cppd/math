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

std::filesystem::path file_name_with_extension(std::filesystem::path file_name)
{
        std::string extension = generic_utf8_filename(file_name.extension());
        if (!extension.empty() && extension[0] == '.')
        {
                check_write_format_support(extension.substr(1));
                return file_name;
        }

        check_write_format_support(DEFAULT_WRITE_FORMAT);
        return file_name.replace_extension(DEFAULT_WRITE_FORMAT);
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
        if (!q_image.save(file_name.c_str()))
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
        if (!q_image.save(file_name.c_str()))
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
        if (!q_image.save(file_name.c_str()))
        {
                error("Error saving pixels to the file " + file_name);
        }
}
}

void save_image_to_file(const std::filesystem::path& file_name, const ImageView<2>& image_view)
{
        std::string f = generic_utf8_filename(file_name_with_extension(file_name));

        int width = image_view.size[0];
        int height = image_view.size[1];

        if (image_view.pixels.size() != 1ull * width * height * format_pixel_size_in_bytes(image_view.color_format))
        {
                error("Error image data size");
        }

        if (format_component_count(image_view.color_format) == 1)
        {
                save_1(f, image_view);
        }
        else if (format_component_count(image_view.color_format) == 3)
        {
                save_3(f, image_view);
        }
        else if (format_component_count(image_view.color_format) == 4)
        {
                save_4(f, image_view);
        }
        else
        {
                error("Image format " + format_to_string(image_view.color_format)
                      + " is not supported for saving to file");
        }
}

void save_image_to_files(
        const std::filesystem::path& directory,
        const std::string& file_format,
        const ImageView<3>& image_view)
{
        if (!std::filesystem::is_directory(directory))
        {
                error("Directory required for 3D image saving: " + generic_utf8_filename(directory));
        }

        const int width = image_view.size[0];
        const int height = image_view.size[1];
        const int image_count = image_view.size[2];
        const size_t image_2d_bytes =
                static_cast<size_t>(format_pixel_size_in_bytes(image_view.color_format)) * width * height;

        ASSERT(image_2d_bytes * image_count == image_view.pixels.size());

        const int image_number_width = std::floor(std::log10(std::max(image_count - 1, 1))) + 1;

        std::ostringstream oss;
        oss << std::setfill('0');
        size_t offset = 0;
        std::string extension = "." + file_format;
        for (int i = 0; i < image_count; ++i)
        {
                oss.str(std::string());
                oss << std::setw(image_number_width) << i << extension;

                image::ImageView<2> image_view_2d(
                        {width, height}, image_view.color_format,
                        std::span(&image_view.pixels[offset], image_2d_bytes));

                image::save_image_to_file(directory / path_from_utf8(oss.str()), image_view_2d);

                offset += image_2d_bytes;
        }
}

void load_image_from_file_rgba(const std::filesystem::path& file_name, Image<2>* image)
{
        std::string f = generic_utf8_filename(file_name);
        QImage q_image;
        if (!q_image.load(f.c_str()) || q_image.width() < 1 || q_image.height() < 1)
        {
                error("Error loading image from the file " + f);
        }

        if (q_image.format() != QImage::Format_ARGB32)
        {
                q_image = q_image.convertToFormat(QImage::Format_ARGB32);
        }

        image->size[0] = q_image.width();
        image->size[1] = q_image.height();

        int width = q_image.width();
        int height = q_image.height();

        image->pixels.resize(4ull * width * height);

        size_t pixel = 0;
        for (int row = 0; row < height; ++row)
        {
                const QRgb* image_line = reinterpret_cast<const QRgb*>(q_image.constScanLine(row));
                for (int col = 0; col < width; ++col)
                {
                        const QRgb& c = image_line[col];
                        uint8_t r = qRed(c);
                        uint8_t g = qGreen(c);
                        uint8_t b = qBlue(c);
                        uint8_t a = qAlpha(c);
                        std::memcpy(&(image->pixels)[pixel++], &r, 1);
                        std::memcpy(&(image->pixels)[pixel++], &g, 1);
                        std::memcpy(&(image->pixels)[pixel++], &b, 1);
                        std::memcpy(&(image->pixels)[pixel++], &a, 1);
                }
        }

        image->color_format = ColorFormat::R8G8B8A8_SRGB;
}
}
