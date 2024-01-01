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

#include "swap.h"

#include "format.h"

#include <src/com/error.h>
#include <src/com/print.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

namespace ns::image
{
namespace
{
template <typename T, std::size_t COUNT>
void swap_rb(const std::span<std::byte> bytes)
{
        static_assert(COUNT >= 3);

        constexpr std::size_t SIZE = sizeof(T) * COUNT;

        if (bytes.size() % SIZE != 0)
        {
                error("Error data size " + to_string(bytes.size()) + " for color component swapping");
        }

        std::byte* ptr = bytes.data();
        const std::byte* const end = ptr + bytes.size();
        while (ptr != end)
        {
                std::array<T, 3> data;
                std::memcpy(data.data(), ptr, sizeof(T) * 3);
                std::swap(data[0], data[2]);
                std::memcpy(ptr, data.data(), sizeof(T) * 3);
                ptr += SIZE;
        }
}
}

void swap_rb(const ColorFormat color_format, const std::span<std::byte>& bytes)
{
        switch (color_format)
        {
        case ColorFormat::R8G8B8_SRGB:
                swap_rb<std::uint8_t, 3>(bytes);
                return;
        case ColorFormat::R8G8B8A8_SRGB:
        case ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
                swap_rb<std::uint8_t, 4>(bytes);
                return;
        case ColorFormat::R16G16B16:
        case ColorFormat::R16G16B16_SRGB:
                swap_rb<std::uint16_t, 3>(bytes);
                return;
        case ColorFormat::R16G16B16A16:
        case ColorFormat::R16G16B16A16_SRGB:
        case ColorFormat::R16G16B16A16_PREMULTIPLIED:
                swap_rb<std::uint16_t, 4>(bytes);
                return;
        case ColorFormat::R32G32B32:
                swap_rb<float, 3>(bytes);
                return;
        case ColorFormat::R32G32B32A32:
        case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                swap_rb<float, 4>(bytes);
                return;
        case ColorFormat::R8_SRGB:
        case ColorFormat::R16:
        case ColorFormat::R32:
                break;
        }
        error("Unsupported image format " + format_to_string(color_format) + " for color component swapping");
}
}
