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

#include "normalize.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>

#include <cmath>
#include <span>

namespace ns::image
{
namespace
{
template <std::size_t N, std::size_t COMPONENT_COUNT>
[[nodiscard]] float find_max(const std::vector<std::byte>* const bytes)
{
        static_assert(N > 0 && N <= COMPONENT_COUNT);

        static constexpr std::size_t COLOR_SIZE = N * sizeof(float);
        static constexpr std::size_t PIXEL_SIZE = COMPONENT_COUNT * sizeof(float);

        ASSERT(bytes->size() % PIXEL_SIZE == 0);

        float max = Limits<float>::lowest();
        const std::byte* ptr = bytes->data();
        const std::byte* const end = bytes->data() + bytes->size();

        while (ptr != end)
        {
                std::array<float, N> pixel;
                static_assert(std::span<float>(pixel).size_bytes() == COLOR_SIZE);
                std::memcpy(pixel.data(), ptr, COLOR_SIZE);
                for (std::size_t i = 0; i < N; ++i)
                {
                        if (std::isfinite(pixel[i]))
                        {
                                max = std::max(max, pixel[i]);
                        }
                }
                ptr += PIXEL_SIZE;
        }

        ASSERT(ptr == bytes->data() + bytes->size());
        ASSERT(std::isfinite(max));

        return max;
}

template <std::size_t N, std::size_t COMPONENT_COUNT>
void normalize(const float max, std::vector<std::byte>* const bytes)
{
        static_assert(N > 0 && N <= COMPONENT_COUNT);

        static constexpr std::size_t COLOR_SIZE = N * sizeof(float);
        static constexpr std::size_t PIXEL_SIZE = COMPONENT_COUNT * sizeof(float);

        ASSERT(bytes->size() % PIXEL_SIZE == 0);

        std::byte* ptr = bytes->data();
        const std::byte* const end = bytes->data() + bytes->size();

        while (ptr != end)
        {
                std::array<float, N> pixel;
                static_assert(std::span<float>(pixel).size_bytes() == COLOR_SIZE);
                std::memcpy(pixel.data(), ptr, COLOR_SIZE);
                for (std::size_t i = 0; i < N; ++i)
                {
                        pixel[i] /= max;
                }
                std::memcpy(ptr, pixel.data(), COLOR_SIZE);
                ptr += PIXEL_SIZE;
        }

        ASSERT(ptr == bytes->data() + bytes->size());
}

template <std::size_t N, std::size_t COMPONENT_COUNT>
void normalize(std::vector<std::byte>* const bytes)
{
        static_assert(N > 0 && N <= COMPONENT_COUNT);

        static constexpr std::size_t PIXEL_SIZE = COMPONENT_COUNT * sizeof(float);

        if (bytes->size() % PIXEL_SIZE != 0)
        {
                error("Error size " + to_string(bytes->size()) + " for normalizing " + to_string(COMPONENT_COUNT)
                      + "-component pixels");
        }

        const float max = find_max<N, COMPONENT_COUNT>(bytes);

        if (!(max > 0 && max != 1))
        {
                return;
        }

        normalize<N, COMPONENT_COUNT>(max, bytes);
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
                normalize<1, 1>(bytes);
                return;
        case ColorFormat::R32G32B32:
                normalize<3, 3>(bytes);
                return;
        case ColorFormat::R32G32B32A32:
        case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                normalize<3, 4>(bytes);
                return;
        }
        unknown_color_format_error(color_format);
}
}
