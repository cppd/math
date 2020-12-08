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

#include "alpha.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>

namespace image
{
namespace
{
template <typename T>
void add_alpha(const std::span<const std::byte>& bytes3, const std::span<std::byte>& bytes4, T alpha)
{
        if (bytes3.size() % (3 * sizeof(T)) != 0)
        {
                error("Error RGB data size " + to_string(bytes3.size()) + " for adding alpha");
        }
        if (bytes4.size() % (4 * sizeof(T)) != 0)
        {
                error("Error RGBA data size " + to_string(bytes4.size()) + " for adding alpha");
        }
        if ((bytes4.size() / 4) * 3 != bytes3.size() || (bytes3.size() / 3) * 4 != bytes4.size())
        {
                error("Error sizes for adding alpha: RGB data size = " + to_string(bytes3.size())
                      + ", RGBA data size = " + to_string(bytes4.size()));
        }

        static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>);
        const size_t pixel_count = bytes3.size() / (3 * sizeof(T));
        ASSERT(pixel_count * (3 * sizeof(T)) == bytes3.size());

        const std::byte* src = bytes3.data();
        std::byte* dst = bytes4.data();
        for (size_t i = 0; i < pixel_count; ++i)
        {
                std::memcpy(dst, src, 3 * sizeof(T));
                dst += 3 * sizeof(T);
                std::memcpy(dst, &alpha, sizeof(T));
                dst += sizeof(T);
                src += 3 * sizeof(T);
        }
}

void add_alpha(
        ColorFormat color_format,
        const std::span<const std::byte>& bytes3,
        const std::span<std::byte>& bytes4,
        float alpha)
{
        if (format_component_count(color_format) != 3)
        {
                error("Error image format " + format_to_string(color_format) + " for adding alpha");
        }
        alpha = std::clamp(alpha, 0.0f, 1.0f);
        if (color_format == ColorFormat::R16G16B16)
        {
                add_alpha<uint16_t>(bytes3, bytes4, std::lround(alpha * limits<uint16_t>::max()));
        }
        else if (color_format == ColorFormat::R32G32B32)
        {
                add_alpha<float>(bytes3, bytes4, alpha);
        }
        else if (color_format == ColorFormat::R8G8B8_SRGB)
        {
                add_alpha<uint8_t>(bytes3, bytes4, std::lround(alpha * limits<uint8_t>::max()));
        }
        else
        {
                error("Unsupported image format " + format_to_string(color_format) + " for adding alpha");
        }
}
}

std::vector<std::byte> add_alpha(ColorFormat color_format, const std::span<const std::byte>& bytes, float alpha)
{
        std::vector<std::byte> bytes4;
        bytes4.resize((bytes.size() / 3) * 4);
        add_alpha(color_format, bytes, bytes4, alpha);
        return bytes4;
}
}
