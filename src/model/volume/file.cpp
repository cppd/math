/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "directory.h"

#include <src/com/alg.h>
#include <src/com/arrays.h>
#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/file/path.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/image/file_load.h>
#include <src/image/file_save.h>
#include <src/image/flip.h>
#include <src/image/format.h>
#include <src/image/image.h>
#include <src/progress/progress.h>
#include <src/settings/instantiation.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <iomanip>
#include <span>
#include <sstream>
#include <string>
#include <vector>

namespace ns::model::volume
{
namespace
{
int max_digit_count_zero_based(const int count)
{
        const double max_number = std::max(count - 1, 1);
        return 1 + std::floor(std::log10(max_number));
}

template <std::size_t N>
void save_to_images(
        const std::filesystem::path& directory,
        const image::ImageView<N>& image_view,
        progress::Ratio* const progress,
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

        const std::array<int, N - 1> image_size_n_1 = del_elem(image_view.size, N - 1);

        for (int i = 0; i < image_view.size[N - 1]; ++i, ptr += size_in_bytes)
        {
                oss.str(std::string());
                oss << std::setw(digit_count) << i;

                const image::ImageView<N - 1> image_view_n_1(
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
std::vector<std::string> read_sorted_names(const std::filesystem::path& directory, const std::array<int, N>& image_size)
{
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
                const std::string s = N >= 4 ? "Directories" : "Files";
                error(s + " not found in directory " + generic_utf8_filename(directory));
        }

        if (names.size() != static_cast<unsigned>(image_size[N - 1]))
        {
                const std::string s = N >= 4 ? "directory" : "file";
                error("Expected " + s + " count " + to_string(image_size[N - 1]) + ", found " + to_string(names.size())
                      + " in " + generic_utf8_filename(directory));
        }

        std::sort(names.begin(), names.end());

        return names;
}

template <std::size_t N>
void load_from_images(
        const std::filesystem::path& directory,
        const image::ColorFormat image_format,
        const std::array<int, N>& image_size,
        const std::span<std::byte> image_bytes,
        progress::Ratio* const progress,
        unsigned* const current,
        const unsigned count)
{
        static_assert(N >= 3);

        const std::vector<std::string> names = read_sorted_names(directory, image_size);

        const std::size_t size_in_bytes = image_bytes.size() / names.size();
        std::byte* ptr = image_bytes.data();

        ASSERT(image_bytes.size() == size_in_bytes * names.size());
        ASSERT(static_cast<long long>(image_bytes.size())
               == image::format_pixel_size_in_bytes(image_format) * multiply_all<long long>(image_size));

        const std::array<int, N - 1> image_size_n_1 = del_elem(image_size, N - 1);

        for (std::size_t i = 0; i < names.size(); ++i, ptr += size_in_bytes)
        {
                const std::filesystem::path entry_path = directory / path_from_utf8(names[i]);
                const std::span span(ptr, size_in_bytes);
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
        auto directory_info = read_directory_info(directory);
        if (!directory_info)
        {
                error("Image files or directories not found in " + generic_utf8_filename(directory));
        }

        const std::filesystem::path path_to_first = directory / path_from_utf8(directory_info->first);

        switch (directory_info->type)
        {
        case ContentType::DIRECTORIES:
        {
                size->push_back(directory_info->count);
                directory_info.reset();
                find_info(path_to_first, size, format);
                return;
        }
        case ContentType::FILES:
        {
                const image::Info file_info = image::file_info(path_to_first);
                const auto [width, height] = file_info.size;
                size->push_back(directory_info->count);
                size->push_back(height);
                size->push_back(width);
                *format = file_info.format;
                return;
        }
        }
        error_fatal("Unknown content type " + to_string(enum_to_int(directory_info->type)));
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
void save_to_images(const Path& path, const image::ImageView<N>& image_view, progress::Ratio* const progress)
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
image::Image<N> load(const Path& path, progress::Ratio* const progress)
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

#define TEMPLATE(N)                                                                                                 \
        template void save_to_images(const std::filesystem::path&, const image::ImageView<(N)>&, progress::Ratio*); \
        template image::Image<(N)> load<(N)>(const std::filesystem::path&, progress::Ratio*);

TEMPLATE_INSTANTIATION_N(TEMPLATE)
}
