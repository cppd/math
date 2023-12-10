/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "max.h"

#include "format.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>

namespace ns::image
{
namespace
{
template <typename T>
[[nodiscard]] bool finite(const T v)
{
        static_assert(std::is_arithmetic_v<T>);
        if constexpr (!std::is_floating_point_v<T>)
        {
                return true;
        }
        else
        {
                return std::isfinite(v);
        }
}

template <typename T, std::size_t COLOR_COUNT, std::size_t COMPONENT_COUNT>
[[nodiscard]] std::optional<T> max(const std::span<const std::byte> bytes)
{
        static_assert(COLOR_COUNT > 0 && COLOR_COUNT <= COMPONENT_COUNT);

        static constexpr std::size_t COLOR_SIZE = COLOR_COUNT * sizeof(T);
        static constexpr std::size_t PIXEL_SIZE = COMPONENT_COUNT * sizeof(T);

        if (bytes.size() % PIXEL_SIZE != 0)
        {
                error("Error size " + to_string(bytes.size()) + " for finding maximum in " + to_string(COMPONENT_COUNT)
                      + "-component pixels with component size " + to_string(sizeof(T)));
        }

        if (bytes.empty())
        {
                return std::nullopt;
        }

        static constexpr T MIN = Limits<T>::lowest();

        T max = MIN;
        const std::byte* ptr = bytes.data();
        const std::byte* const end = bytes.data() + bytes.size();

        while (ptr != end)
        {
                std::array<T, COLOR_COUNT> pixel;
                static_assert(std::span<T>(pixel).size_bytes() == COLOR_SIZE);
                std::memcpy(pixel.data(), ptr, COLOR_SIZE);
                for (std::size_t i = 0; i < COLOR_COUNT; ++i)
                {
                        if (finite(pixel[i]))
                        {
                                max = std::max(max, pixel[i]);
                        }
                }
                ptr += PIXEL_SIZE;
        }

        ASSERT(ptr == bytes.data() + bytes.size());
        ASSERT(finite(max));

        if (!std::is_floating_point_v<T> || max != MIN)
        {
                return max;
        }
        return std::nullopt;
}
}

std::optional<double> max(const ColorFormat color_format, const std::span<const std::byte> bytes)
{
        switch (color_format)
        {
        case ColorFormat::R8_SRGB:
                return max<std::uint8_t, 1, 1>(bytes);
        case ColorFormat::R8G8B8_SRGB:
                return max<std::uint8_t, 3, 3>(bytes);
        case ColorFormat::R8G8B8A8_SRGB:
        case ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
                return max<std::uint8_t, 3, 4>(bytes);
        case ColorFormat::R16:
                return max<std::uint16_t, 1, 1>(bytes);
        case ColorFormat::R16G16B16:
        case ColorFormat::R16G16B16_SRGB:
                return max<std::uint16_t, 3, 3>(bytes);
        case ColorFormat::R16G16B16A16:
        case ColorFormat::R16G16B16A16_SRGB:
        case ColorFormat::R16G16B16A16_PREMULTIPLIED:
                return max<std::uint16_t, 3, 4>(bytes);
        case ColorFormat::R32:
                return max<float, 1, 1>(bytes);
        case ColorFormat::R32G32B32:
                return max<float, 3, 3>(bytes);
        case ColorFormat::R32G32B32A32:
        case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                return max<float, 3, 4>(bytes);
        }
        unknown_color_format_error(color_format);
}
}
