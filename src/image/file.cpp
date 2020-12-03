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
        ContentType type;
        std::vector<std::string> entries;
};

std::optional<DirectoryContent> read_directory_ascii_content(const std::filesystem::path& directory)
{
        if (!std::filesystem::is_directory(directory))
        {
                error("Directory not found " + generic_utf8_filename(directory));
        }

        std::vector<std::string> entries;

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
                entries.push_back(generic_utf8_filename(entry.path().filename()));
                if (!ascii::is_ascii(entries.back()))
                {
                        error("Directory entry does not have only ASCII encoding "
                              + generic_utf8_filename(entry.path()));
                }
        }
        if (entries.empty())
        {
                return std::nullopt;
        }
        ASSERT(files != directories);
        if (files == directories)
        {
                return std::nullopt;
        }

        return DirectoryContent{
                .type = (files ? ContentType::Files : ContentType::Directories),
                .entries = std::move(entries)};
}

std::tuple<int, int> image_width_height(const std::filesystem::path& file_name)
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

void load_r8g8b8a8(
        const std::filesystem::path& file_name,
        const std::array<int, 2>& image_size,
        const std::span<std::byte>& pixels)
{
        std::string file = generic_utf8_filename(file_name);
        QImage q_image;
        if (!q_image.load(QString::fromStdString(file)))
        {
                error("Error loading image from the file " + file);
        }
        if (q_image.width() != image_size[0] || q_image.height() != image_size[1])
        {
                error("Expected image size (" + to_string(image_size[0]) + ", " + to_string(image_size[1])
                      + "), found size (" + to_string(q_image.width()) + ", " + to_string(q_image.height())
                      + ") in file " + file);
        }

        load_r8g8b8a8(std::move(q_image), pixels);
}

Image<2> load_r8g8b8a8(const std::filesystem::path& file_name)
{
        std::string file = generic_utf8_filename(file_name);
        QImage q_image;
        if (!q_image.load(QString::fromStdString(file)) || q_image.width() < 1 || q_image.height() < 1)
        {
                error("Error loading image from the file " + file);
        }

        Image<2> image;
        image.color_format = ColorFormat::R8G8B8A8_SRGB;
        image.size[0] = q_image.width();
        image.size[1] = q_image.height();
        image.pixels.resize(4ull * image.size[0] * image.size[1]);

        load_r8g8b8a8(std::move(q_image), image.pixels);

        return image;
}

int max_digit_count_zero_based(int count)
{
        const int max_number = count - 1;
        return std::floor(std::log10(std::max(max_number, 1))) + 1;
}

template <size_t N>
std::enable_if_t<N >= 3> save_image_to_files(
        const std::filesystem::path& directory,
        const std::string& file_format,
        const ImageView<N>& image_view,
        ProgressRatio* progress,
        unsigned* current,
        unsigned count)
{
        static_assert(N >= 3);

        const int digit_count = max_digit_count_zero_based(image_view.size[N - 1]);

        const size_t size_in_bytes = image_view.pixels.size() / image_view.size[N - 1];
        const std::byte* ptr = image_view.pixels.data();

        ASSERT(image_view.pixels.size() == size_in_bytes * image_view.size[N - 1]);
        ASSERT(static_cast<long long>(image_view.pixels.size())
               == format_pixel_size_in_bytes(image_view.color_format) * multiply_all<long long>(image_view.size));

        std::ostringstream oss;
        oss << std::setfill('0');

        std::array<int, N - 1> image_size_n_1 = del_elem(image_view.size, N - 1);

        for (int i = 0; i < image_view.size[N - 1]; ++i, ptr += size_in_bytes)
        {
                oss.str(std::string());
                oss << std::setw(digit_count) << i;

                ImageView<N - 1> image_view_n_1(image_size_n_1, image_view.color_format, std::span(ptr, size_in_bytes));

                if constexpr (N >= 4)
                {
                        const std::filesystem::path directory_n_1 = directory / path_from_utf8(oss.str());
                        std::filesystem::create_directory(directory_n_1);
                        save_image_to_files(directory_n_1, file_format, image_view_n_1, progress, current, count);
                }
                else
                {
                        oss << "." << file_format;
                        save_image_to_file(directory / path_from_utf8(oss.str()), image_view_n_1);

                        progress->set(++(*current), count);
                }
        }
}

template <size_t N>
std::enable_if_t<N >= 3> load_r8g8b8a8_from_files(
        const std::filesystem::path& directory,
        const std::array<int, N>& image_size,
        const std::span<std::byte>& image_bytes,
        ProgressRatio* progress,
        unsigned* current,
        unsigned count)
{
        static_assert(N >= 3);

        std::vector<std::string> names = read_ascii_file_names_from_directory(directory);
        if (names.empty())
        {
                std::string s = N >= 4 ? "directories" : "files";
                error("No " + s + " found in directory " + generic_utf8_filename(directory));
        }
        if (names.size() != static_cast<unsigned>(image_size[N - 1]))
        {
                std::string s = N >= 4 ? "directory" : "file";
                error("Expected " + s + " count " + to_string(image_size[N - 1]) + ", found " + to_string(names.size())
                      + " in " + generic_utf8_filename(directory));
        }
        std::sort(names.begin(), names.end());

        const size_t size_in_bytes = image_bytes.size() / names.size();
        std::byte* ptr = image_bytes.data();

        ASSERT(image_bytes.size() == size_in_bytes * names.size());
        ASSERT(image_bytes.size() == 4ull * multiply_all<long long>(image_size));

        std::array<int, N - 1> image_size_n_1 = del_elem(image_size, N - 1);

        for (size_t i = 0; i < names.size(); ++i, ptr += size_in_bytes)
        {
                std::filesystem::path entry_path = directory / path_from_utf8(names[i]);
                std::span span(ptr, size_in_bytes);
                if constexpr (N >= 4)
                {
                        if (!std::filesystem::is_directory(entry_path))
                        {
                                error("Path expected to be a directory " + generic_utf8_filename(entry_path));
                        }
                        load_r8g8b8a8_from_files(entry_path, image_size_n_1, span, progress, current, count);
                }
                else
                {
                        load_r8g8b8a8(entry_path, image_size_n_1, span);

                        progress->set(++(*current), count);
                }
        }
}

void find_image_size(const std::filesystem::path& directory, std::vector<int>* size)
{
        std::optional<DirectoryContent> content = read_directory_ascii_content(directory);
        if (!content || content->entries.empty())
        {
                error("Image files or directories not found in " + generic_utf8_filename(directory));
        }

        const std::filesystem::path first =
                directory / path_from_utf8(*std::min_element(content->entries.cbegin(), content->entries.cend()));

        switch (content->type)
        {
        case ContentType::Directories:
        {
                size->push_back(content->entries.size());
                content.reset();
                find_image_size(first, size);
                return;
        }
        case ContentType::Files:
        {
                const auto [width, height] = image_width_height(first);
                size->push_back(content->entries.size());
                size->push_back(height);
                size->push_back(width);
                return;
        }
        }
        error_fatal("Unknown content type " + to_string(static_cast<long long>(content->type)));
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

Image<2> load_image_from_file_rgba(const std::filesystem::path& file_name)
{
        return load_r8g8b8a8(file_name);
}

std::vector<int> find_image_size(const std::filesystem::path& directory)
{
        std::vector<int> size;
        find_image_size(directory, &size);
        if (size.size() < 3)
        {
                error("Image dimension " + to_string(size.size()) + " is less than 3");
        }
        std::reverse(size.begin(), size.end());
        if (!all_positive(size))
        {
                error("Image dimensions " + to_string(size) + " are not positive");
        }
        return size;
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
        long long image_count = multiply_all<long long>(image_view.size) / image_view.size[0] / image_view.size[1];
        if (static_cast<unsigned long long>(image_count) > limits<unsigned>::max())
        {
                error("Too many images to save, image size " + to_string(image_view.size));
        }
        unsigned current = 0;
        save_image_to_files(directory, file_format, image_view, progress, &current, image_count);
}

template <size_t N>
std::enable_if_t<N >= 3, Image<N>> load_images_from_files_rgba(
        const std::filesystem::path& directory,
        ProgressRatio* progress)
{
        const std::vector<int> image_size = find_image_size(directory);
        if (image_size.size() != N)
        {
                error("Error loading " + to_string(N) + "-image, found image dimension " + to_string(image_size.size())
                      + " in " + generic_utf8_filename(directory));
        }

        long long pixel_count = multiply_all<long long>(image_size);

        Image<N> image;
        image.color_format = ColorFormat::R8G8B8A8_SRGB;
        for (size_t i = 0; i < N; ++i)
        {
                image.size[i] = image_size[i];
        }
        image.pixels.resize(4 * pixel_count);

        long long image_count = pixel_count / image.size[0] / image.size[1];
        if (static_cast<unsigned long long>(image_count) > limits<unsigned>::max())
        {
                error("Too many images to load, image size " + to_string(image_size));
        }
        unsigned current = 0;
        load_r8g8b8a8_from_files(directory, image.size, image.pixels, progress, &current, image_count);

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

template Image<3> load_images_from_files_rgba<3>(const std::filesystem::path& directory, ProgressRatio* progress);
template Image<4> load_images_from_files_rgba<4>(const std::filesystem::path& directory, ProgressRatio* progress);

}
