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

#pragma once

#include "image.h"

#include <src/com/alg.h>
#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/progress/progress.h>
#include <src/utility/file/path.h>

#include <cmath>
#include <sstream>

namespace image
{
namespace implementation
{
void save_image_to_files(
        const std::filesystem::path& directory,
        const std::string& file_format,
        const ImageView<3>& image_view,
        ProgressRatio* progress,
        unsigned* current,
        unsigned count);

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
}

void save_image_to_file(const std::filesystem::path& file_name, const ImageView<2>& image_view);

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
        implementation::save_image_to_files(directory, file_format, image_view, progress, &current, count);
}
//

void load_image_from_file_rgba(const std::filesystem::path& file_name, Image<2>* image);

void load_image_from_files_rgba(const std::filesystem::path& directory, Image<3>* image);
}
