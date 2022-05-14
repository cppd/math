/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <src/com/alg.h>
#include <src/com/arrays.h>
#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/print.h>
#include <src/com/string/ascii.h>
#include <src/image/file_load.h>
#include <src/image/file_save.h>
#include <src/image/flip.h>

#include <cmath>
#include <filesystem>
#include <optional>
#include <sstream>
#include <vector>

namespace ns::model::volume
{
namespace
{
int max_digit_count_zero_based(const int count)
{
        const int max_number = count - 1;
        return std::floor(std::log10(std::max(max_number, 1))) + 1;
}

enum class ContentType
{
        FILES,
        DIRECTORIES
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
                .type = (files ? ContentType::FILES : ContentType::DIRECTORIES),
                .entries = std::move(entries)};
}

std::vector<std::string> read_directories(const std::filesystem::path& directory)
{
        std::optional<DirectoryContent> content = read_directory_ascii_content(directory);
        if (!content || content->entries.empty())
        {
                error("Directories not found in " + generic_utf8_filename(directory));
        }
        if (content->type != ContentType::DIRECTORIES)
        {
                error("Directory " + generic_utf8_filename(directory) + " does not contain only directories");
        }
        return std::move(content->entries);
}

std::vector<std::string> read_files(const std::filesystem::path& directory)
{
        std::optional<DirectoryContent> content = read_directory_ascii_content(directory);
        if (!content || content->entries.empty())
        {
                error("Files not found in " + generic_utf8_filename(directory));
        }
        if (content->type != ContentType::FILES)
        {
                error("Directory " + generic_utf8_filename(directory) + " does not contain only files");
        }
        return std::move(content->entries);
}

template <std::size_t N>
void save_to_images(
        const std::filesystem::path& directory,
        const image::ImageView<N>& image_view,
        ProgressRatio* const progress,
        unsigned* const current,
        const unsigned count)
{
        static_assert(N >= 3);

        const int digit_count = max_digit_count_zero_based(image_view.size[N - 1]);

        const std::size_t size_in_bytes = image_view.pixels.size() / image_view.size[N - 1];
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

                image::ImageView<N - 1> image_view_n_1(
                        image_size_n_1, image_view.color_format, std::span(ptr, size_in_bytes));

                if constexpr (N >= 4)
                {
                        const std::filesystem::path directory_n_1 = directory / path_from_utf8(oss.str());
                        std::filesystem::create_directory(directory_n_1);
                        save_to_images(directory_n_1, image_view_n_1, progress, current, count);
                }
                else
                {
                        image::save(directory / path_from_utf8(oss.str()), image_view_n_1);

                        progress->set(++(*current), count);
                }
        }
}

template <std::size_t N>
void load_from_images(
        const std::filesystem::path& directory,
        const image::ColorFormat image_format,
        const std::array<int, N>& image_size,
        const std::span<std::byte>& image_bytes,
        ProgressRatio* const progress,
        unsigned* const current,
        const unsigned count)
{
        static_assert(N >= 3);

        std::vector<std::string> names;
        if (N >= 4)
        {
                names = read_directories(directory);
        }
        else
        {
                names = read_files(directory);
        }

        if (names.empty())
        {
                std::string s = N >= 4 ? "Directories" : "Files";
                error(s + " not found in directory " + generic_utf8_filename(directory));
        }
        if (names.size() != static_cast<unsigned>(image_size[N - 1]))
        {
                std::string s = N >= 4 ? "directory" : "file";
                error("Expected " + s + " count " + to_string(image_size[N - 1]) + ", found " + to_string(names.size())
                      + " in " + generic_utf8_filename(directory));
        }

        std::sort(names.begin(), names.end());

        const std::size_t size_in_bytes = image_bytes.size() / names.size();
        std::byte* ptr = image_bytes.data();

        ASSERT(image_bytes.size() == size_in_bytes * names.size());
        ASSERT(static_cast<long long>(image_bytes.size())
               == image::format_pixel_size_in_bytes(image_format) * multiply_all<long long>(image_size));

        std::array<int, N - 1> image_size_n_1 = del_elem(image_size, N - 1);

        for (std::size_t i = 0; i < names.size(); ++i, ptr += size_in_bytes)
        {
                std::filesystem::path entry_path = directory / path_from_utf8(names[i]);
                std::span span(ptr, size_in_bytes);
                if constexpr (N >= 4)
                {
                        if (!std::filesystem::is_directory(entry_path))
                        {
                                error("Path expected to be a directory " + generic_utf8_filename(entry_path));
                        }
                        load_from_images(entry_path, image_format, image_size_n_1, span, progress, current, count);
                }
                else
                {
                        image::load(entry_path, image_format, image_size_n_1, span);

                        progress->set(++(*current), count);
                }
        }
}

void find_info(const std::filesystem::path& directory, std::vector<int>* const size, image::ColorFormat* const format)
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
        case ContentType::DIRECTORIES:
        {
                size->push_back(content->entries.size());
                content.reset();
                find_info(first, size, format);
                return;
        }
        case ContentType::FILES:
        {
                image::Info info = image::file_info(first);
                const auto [width, height] = info.size;
                size->push_back(content->entries.size());
                size->push_back(height);
                size->push_back(width);
                *format = info.format;
                return;
        }
        }
        error_fatal("Unknown content type " + to_string(enum_to_int(content->type)));
}
}

template <typename Path>
VolumeInfo volume_info(const Path& path)
{
        static_assert(std::is_same_v<Path, std::filesystem::path>);

        VolumeInfo info;
        find_info(path, &info.size, &info.format);
        if (info.size.size() < 3)
        {
                error("Image dimension " + to_string(info.size.size()) + " is less than 3");
        }
        std::reverse(info.size.begin(), info.size.end());
        if (!all_positive(info.size))
        {
                error("Image dimensions " + to_string(info.size) + " are not positive");
        }
        return info;
}

template <std::size_t N, typename Path>
void save_to_images(const Path& path, const image::ImageView<N>& image_view, ProgressRatio* const progress)
{
        static_assert(std::is_same_v<Path, std::filesystem::path>);

        if (!all_positive(image_view.size))
        {
                error("Image size is not positive: " + to_string(image_view.size));
        }

        const long long image_count =
                multiply_all<long long>(image_view.size) / image_view.size[0] / image_view.size[1];
        if (static_cast<unsigned long long>(image_count) > Limits<unsigned>::max())
        {
                error("Too many images to save, image size " + to_string(image_view.size));
        }

        unsigned current = 0;
        save_to_images(path, image_view, progress, &current, image_count);
}

template <std::size_t N, typename Path>
image::Image<N> load(const Path& path, ProgressRatio* const progress)
{
        static_assert(std::is_same_v<Path, std::filesystem::path>);

        const VolumeInfo info = volume_info(path);
        if (info.size.size() != N)
        {
                error("Error loading " + to_string(N) + "-image, found image dimension " + to_string(info.size.size())
                      + " in " + generic_utf8_filename(path));
        }

        const auto pixel_count = multiply_all<long long>(info.size);

        image::Image<N> image;
        image.color_format = info.format;
        for (std::size_t i = 0; i < N; ++i)
        {
                image.size[i] = info.size[i];
        }
        image.pixels.resize(image::format_pixel_size_in_bytes(info.format) * pixel_count);

        const long long image_count = pixel_count / image.size[0] / image.size[1];
        if (static_cast<unsigned long long>(image_count) > Limits<unsigned>::max())
        {
                error("Too many images to load, image size " + to_string(info.size));
        }

        unsigned current = 0;
        load_from_images(path, image.color_format, image.size, image.pixels, progress, &current, image_count);

        image::flip_vertically(&image);

        return image;
}

template VolumeInfo volume_info(const std::filesystem::path&);

#define FUNCTION_INSTANTIATIONS(N)                                                                                \
        template void save_to_images(const std::filesystem::path&, const image::ImageView<(N)>&, ProgressRatio*); \
        template image::Image<(N)> load<(N)>(const std::filesystem::path&, ProgressRatio*);

FUNCTION_INSTANTIATIONS(3)
FUNCTION_INSTANTIATIONS(4)
FUNCTION_INSTANTIATIONS(5)
}
