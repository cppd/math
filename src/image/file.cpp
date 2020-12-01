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

#include <src/com/alg.h>
#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/utility/file/path.h>
#include <src/utility/string/ascii.h>
#include <src/utility/string/str.h>

#include <QImage>
#include <QImageWriter>
#include <cmath>
#include <cstring>
#include <set>
#include <sstream>

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

std::vector<std::string> read_ascii_file_names_from_directory(const std::filesystem::path& directory)
{
        if (!std::filesystem::is_directory(directory))
        {
                error("Directory not found " + generic_utf8_filename(directory));
        }
        std::vector<std::string> files;
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directory))
        {
                if (!entry.is_regular_file())
                {
                        error("Non-regular file found " + generic_utf8_filename(entry.path()));
                }
                files.push_back(generic_utf8_filename(entry.path().filename()));
                if (!ascii::is_ascii(files.back()))
                {
                        error("File name does not have only ASCII encoding " + generic_utf8_filename(entry.path()));
                }
        }
        return files;
}

enum class ContentType
{
        Files,
        Directories
};

struct DirectoryContent final
{
        std::optional<ContentType> type;
        std::vector<std::string> entries;
};

DirectoryContent read_directory_ascii_content(const std::filesystem::path& directory)
{
        if (!std::filesystem::is_directory(directory))
        {
                error("Directory not found " + generic_utf8_filename(directory));
        }

        DirectoryContent content;

        bool files = false;
        bool directories = false;
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directory))
        {
                if (entry.is_directory())
                {
                        if (files)
                        {
                                error("Mixed content found in directory " + generic_utf8_filename(directory));
                        }
                        directories = true;
                }
                else if (entry.is_regular_file())
                {
                        if (directories)
                        {
                                error("Mixed content found in directory " + generic_utf8_filename(directory));
                        }
                        files = true;
                }
                else
                {
                        error("Neither directory nor regular file found " + generic_utf8_filename(entry.path()));
                }
                content.entries.push_back(generic_utf8_filename(entry.path().filename()));
                if (!ascii::is_ascii(content.entries.back()))
                {
                        error("Directory entry does not have only ASCII encoding "
                              + generic_utf8_filename(entry.path()));
                }
        }

        ASSERT(content.entries.empty() || (files != directories));
        if (files)
        {
                content.type = ContentType::Files;
        }
        else if (directories)
        {
                content.type = ContentType::Directories;
        }

        return content;
}

std::tuple<int, int> image_size(const std::filesystem::path& file_name)
{
        std::string f = generic_utf8_filename(file_name);
        QImage q_image;
        if (!q_image.load(QString::fromStdString(f)) || q_image.width() < 1 || q_image.height() < 1)
        {
                error("Error loading image from the file " + f);
        }
        return std::tuple<int, int>(q_image.width(), q_image.height());
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

void load_r8g8b8a8(QImage&& image, const std::span<std::byte>& pixels)
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

void save_image_to_files(
        const std::filesystem::path& directory,
        const std::string& file_format,
        const ImageView<3>& image_view,
        ProgressRatio* progress,
        unsigned* current,
        unsigned count)
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

                progress->set(++(*current), count);
        }
}

template <size_t N>
std::enable_if_t<N >= 4> save_image_to_files(
        const std::filesystem::path& directory,
        const std::string& file_format,
        const ImageView<N>& image_view,
        ProgressRatio* progress,
        unsigned* current,
        unsigned count)
{
        static_assert(N >= 4);

        const int image_n_1_count = image_view.size[N - 1];

        const std::array<int, N - 1> image_n_1_size = del_elem(image_view.size, N - 1);
        const size_t image_n_1_size_in_bytes =
                format_pixel_size_in_bytes(image_view.color_format) * (multiply_all<long long>(image_n_1_size));

        const int number_width = std::floor(std::log10(std::max(image_n_1_count - 1, 1))) + 1;

        ASSERT(image_n_1_size_in_bytes * image_n_1_count == image_view.pixels.size());

        std::ostringstream oss;
        oss << std::setfill('0');
        size_t offset = 0;
        for (int i = 0; i < image_n_1_count; ++i)
        {
                oss.str(std::string());
                oss << std::setw(number_width) << i;

                const std::filesystem::path directory_n_1 = directory / path_from_utf8(oss.str());
                std::filesystem::create_directory(directory_n_1);

                std::span pixels(image_view.pixels.data() + offset, image_n_1_size_in_bytes);
                save_image_to_files(
                        directory_n_1, file_format, ImageView<N - 1>(image_n_1_size, image_view.color_format, pixels),
                        progress, current, count);

                offset += image_n_1_size_in_bytes;
        }
}

void load_r8g8b8a8_from_files(
        const std::filesystem::path& directory,
        unsigned image_count,
        int width,
        int height,
        const std::span<std::byte>& image_bytes,
        ProgressRatio* progress,
        unsigned* current,
        unsigned count)
{
        std::vector<std::string> files = read_ascii_file_names_from_directory(directory);
        if (files.empty())
        {
                error("No files found in directory " + generic_utf8_filename(directory));
        }
        if (files.size() != image_count)
        {
                error("Expected image count " + to_string(image_count) + ", found " + to_string(files.size()));
        }
        std::sort(files.begin(), files.end());

        ASSERT(image_bytes.size() == 4ull * image_count * width * height);
        const size_t size_2d = image_bytes.size() / image_count;
        std::byte* ptr_2d = image_bytes.data();

        for (const std::string& file : files)
        {
                std::string f = generic_utf8_filename(directory / path_from_utf8(file));
                QImage q_image;
                if (!q_image.load(QString::fromStdString(f)))
                {
                        error("Error loading image from the file " + f);
                }
                if (q_image.width() != width || q_image.height() != height)
                {
                        error("Expected image size " + files.front() + " (" + to_string(width) + ", "
                              + to_string(height) + "), found size (" + to_string(q_image.width()) + ", "
                              + to_string(q_image.height()) + ") in file " + f);
                }

                load_r8g8b8a8(std::move(q_image), std::span(ptr_2d, size_2d));
                ptr_2d += size_2d;

                progress->set(++(*current), count);
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

template <size_t N>
std::enable_if_t<N >= 3> save_image_to_files(
        const std::filesystem::path& directory,
        const std::string& file_format,
        const ImageView<N>& image_view,
        ProgressRatio* progress)
{
        if (!all_positive(image_view.size))
        {
                error("Image size is not positive: " + to_string(image_view.size));
        }
        long long count = multiply_all<long long>(image_view.size) / image_view.size[0] / image_view.size[1];
        if (static_cast<unsigned long long>(count) > limits<unsigned>::max())
        {
                error("Too many images to save, image size " + to_string(image_view.size));
        }
        unsigned current = 0;
        save_image_to_files(directory, file_format, image_view, progress, &current, count);
}

Image<2> load_image_from_file_rgba(const std::filesystem::path& file_name)
{
        std::string f = generic_utf8_filename(file_name);
        QImage q_image;
        if (!q_image.load(QString::fromStdString(f)) || q_image.width() < 1 || q_image.height() < 1)
        {
                error("Error loading image from the file " + f);
        }

        Image<2> image;
        image.color_format = ColorFormat::R8G8B8A8_SRGB;
        image.size[0] = q_image.width();
        image.size[1] = q_image.height();
        image.pixels.resize(4ull * image.size[0] * image.size[1]);

        load_r8g8b8a8(std::move(q_image), image.pixels);

        return image;
}

Image<3> load_image_from_files_rgba(const std::filesystem::path& directory, ProgressRatio* progress)
{
        const DirectoryContent content = read_directory_ascii_content(directory);
        ASSERT(content.type.has_value() != content.entries.empty());

        if (!content.type || *content.type != ContentType::Files || content.entries.empty())
        {
                error("No files found in directory " + generic_utf8_filename(directory));
        }

        if (content.entries.size() > limits<unsigned>::max())
        {
                error("Too many images to load: " + to_string(content.entries.size()));
        }

        Image<3> image;
        image.color_format = ColorFormat::R8G8B8A8_SRGB;
        std::tie(image.size[0], image.size[1]) = image_size(
                directory / path_from_utf8(*std::min_element(content.entries.cbegin(), content.entries.cend())));
        image.size[2] = content.entries.size();
        image.pixels.resize(4 * multiply_all<long long>(image.size));

        unsigned current = 0;
        unsigned count = content.entries.size();

        load_r8g8b8a8_from_files(
                directory, content.entries.size(), image.size[0], image.size[1],
                std::span(image.pixels.data(), image.pixels.size()), progress, &current, count);

        return image;
}

template void save_image_to_files(
        const std::filesystem::path&,
        const std::string&,
        const ImageView<3>&,
        ProgressRatio*);
template void save_image_to_files(
        const std::filesystem::path&,
        const std::string&,
        const ImageView<4>&,
        ProgressRatio*);
}
