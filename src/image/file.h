/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <array>
#include <filesystem>
#include <span>
#include <string_view>

namespace ns::image
{
[[nodiscard]] std::string_view file_extension();

void save(const std::filesystem::path& path, const ImageView<2>& image_view);

struct Info final
{
        std::array<int, 2> size;
        ColorFormat format;
};
[[nodiscard]] Info file_info(const std::filesystem::path& path);

[[nodiscard]] Image<2> load_rgba(const std::filesystem::path& path);

void load(
        const std::filesystem::path& path,
        ColorFormat color_format,
        const std::array<int, 2>& size,
        const std::span<std::byte>& pixels);
}
