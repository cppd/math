/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "normalize.h"

#include "format.h"
#include "max.h"

#include <src/com/error.h>
#include <src/com/print.h>

#include <array>
#include <cstddef>
#include <cstring>
#include <optional>
#include <span>
#include <vector>

namespace ns::image
{
namespace
{
template <std::size_t COLOR_COUNT, std::size_t COMPONENT_COUNT>
void normalize(const ColorFormat color_format, std::vector<std::byte>* const bytes)
{
        static_assert(COLOR_COUNT > 0 && COLOR_COUNT <= COMPONENT_COUNT);

        static constexpr std::size_t COLOR_SIZE = COLOR_COUNT * sizeof(float);
        static constexpr std::size_t PIXEL_SIZE = COMPONENT_COUNT * sizeof(float);

        if (bytes->size() % PIXEL_SIZE != 0)
        {
                error("Error size " + to_string(bytes->size()) + " for normalizing " + to_string(COMPONENT_COUNT)
                      + "-component pixels");
        }

        const auto image_max = max(color_format, *bytes);
        if (!image_max)
        {
                return;
        }

        const float max = *image_max;
        if (!(max > 0 && max != 1))
        {
                return;
        }

        std::byte* ptr = bytes->data();
        const std::byte* const end = bytes->data() + bytes->size();

        while (ptr != end)
        {
                std::array<float, COLOR_COUNT> pixel;
                static_assert(std::span<float>(pixel).size_bytes() == COLOR_SIZE);
                std::memcpy(pixel.data(), ptr, COLOR_SIZE);
                for (std::size_t i = 0; i < COLOR_COUNT; ++i)
                {
                        pixel[i] /= max;
                }
                std::memcpy(ptr, pixel.data(), COLOR_SIZE);
                ptr += PIXEL_SIZE;
        }

        ASSERT(ptr == bytes->data() + bytes->size());
}
}

void normalize(const ColorFormat color_format, std::vector<std::byte>* const bytes)
{
        switch (color_format)
        {
        case ColorFormat::R8_SRGB:
        case ColorFormat::R16:
        case ColorFormat::R8G8B8_SRGB:
        case ColorFormat::R8G8B8A8_SRGB:
        case ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
        case ColorFormat::R16G16B16:
        case ColorFormat::R16G16B16_SRGB:
        case ColorFormat::R16G16B16A16:
        case ColorFormat::R16G16B16A16_SRGB:
        case ColorFormat::R16G16B16A16_PREMULTIPLIED:
                error("Unsupported image format " + format_to_string(color_format) + " for normalizing");
        case ColorFormat::R32:
                normalize<1, 1>(color_format, bytes);
                return;
        case ColorFormat::R32G32B32:
                normalize<3, 3>(color_format, bytes);
                return;
        case ColorFormat::R32G32B32A32:
        case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                normalize<3, 4>(color_format, bytes);
                return;
        }
        unknown_color_format_error(color_format);
}
}
