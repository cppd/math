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

#include <src/image/image.h>
#include <src/progress/progress.h>

#include <filesystem>
#include <string>

namespace volume
{
std::vector<int> find_size(const std::filesystem::path& path);

template <size_t N>
std::enable_if_t<N >= 3> save_to_images(
        const std::filesystem::path& path,
        const std::string& file_format,
        const image::ImageView<N>& image_view,
        ProgressRatio* progress);

template <size_t N>
std::enable_if_t<N >= 3, image::Image<N>> load_rgba(const std::filesystem::path& path, ProgressRatio* progress);
}
