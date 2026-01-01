/*
Copyright (C) 2017-2026 Topological Manifold

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

#include <src/image/format.h>
#include <src/image/image.h>
#include <src/progress/progress.h>

#include <cstddef>
#include <vector>

namespace ns::model::volume
{
struct VolumeInfo final
{
        std::vector<int> size;
        image::ColorFormat format;
};

template <typename Path>
VolumeInfo volume_info(const Path& path);

template <std::size_t N, typename Path>
void save_to_images(const Path& path, const image::ImageView<N>& image_view, progress::Ratio* progress);

template <std::size_t N, typename Path>
image::Image<N> load(const Path& path, progress::Ratio* progress);
}
