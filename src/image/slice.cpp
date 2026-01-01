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

#include "slice.h"

#include "format.h"
#include "image.h"

#include <src/com/alg.h>
#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/global_index.h>
#include <src/com/print.h>
#include <src/settings/instantiation.h>

#include <array>
#include <cstddef>
#include <cstring>
#include <vector>

namespace ns::image
{
namespace
{
template <std::size_t N, std::size_t S>
void check_image_and_slices(const Image<N>& image, const std::array<Slice, S>& slices)
{
        static_assert(S > 0 && S < N);

        for (std::size_t i = 0; i < image.size.size(); ++i)
        {
                if (!(image.size[i] > 0))
                {
                        error("Image size is not positive " + to_string(image.size));
                }
        }

        for (const Slice& s : slices)
        {
                if (!(s.dimension < N))
                {
                        error("Dimension " + to_string(s.dimension) + " is out of the range [0, " + to_string(N) + ")");
                }

                if (!(s.coordinate < static_cast<std::size_t>(image.size[s.dimension])))
                {
                        error("Slice coordinate " + to_string(s.coordinate) + " is out of the range [0, "
                              + to_string(image.size[s.dimension]) + ")");
                }
        }
}

template <std::size_t N, std::size_t S>
std::array<bool, N> find_slice_dimensions(const std::array<Slice, S>& slices)
{
        static_assert(S > 0 && S < N);

        std::array<bool, N> res = make_array_value<bool, N>(false);
        for (std::size_t i = 0; i < S; ++i)
        {
                const std::size_t d = slices[i].dimension;
                if (!(d < N))
                {
                        error("Slice dimension " + to_string(d) + " is out of the range [0, " + to_string(N) + ")");
                }
                if (res[d])
                {
                        error("Not unique slice dimension " + to_string(d));
                }
                res[d] = true;
        }
        return res;
}

template <std::size_t N, std::size_t S>
std::array<int, N - S> create_coordinate_map(const std::array<Slice, S>& slices)
{
        static_assert(S > 0 && S < N);

        const std::array<bool, N> slice_dimensions = find_slice_dimensions<N>(slices);

        std::array<int, N - S> map;
        std::size_t map_i = 0;
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!slice_dimensions[i])
                {
                        map[map_i++] = i;
                }
        }
        ASSERT(map_i == map.size());
        return map;
}

template <int I, std::size_t PIXEL_SIZE, std::size_t N, std::size_t S>
void copy_slice(
        const std::array<int, S>& dst_size,
        const std::array<int, S>& map,
        std::size_t& dst_offset,
        std::vector<std::byte>& dst_pixels,
        const GlobalIndex<N, long long>& src_index,
        const std::vector<std::byte>& src_pixels,
        std::array<int, N>& src_coordinates)
{
        static_assert(I >= -1 && I < static_cast<int>(S) && PIXEL_SIZE > 0 && S > 0 && S < N);

        if constexpr (I >= 0)
        {
                for (int i = 0; i < dst_size[I]; ++i)
                {
                        src_coordinates[map[I]] = i;
                        copy_slice<I - 1, PIXEL_SIZE>(
                                dst_size, map, dst_offset, dst_pixels, src_index, src_pixels, src_coordinates);
                }
        }
        else
        {
                const std::size_t src_offset = PIXEL_SIZE * src_index.compute(src_coordinates);
                ASSERT(src_offset + PIXEL_SIZE <= src_pixels.size());
                ASSERT(dst_offset + PIXEL_SIZE <= dst_pixels.size());
                std::memcpy(&dst_pixels[dst_offset], &src_pixels[src_offset], PIXEL_SIZE);
                dst_offset += PIXEL_SIZE;
        }
}

template <std::size_t PIXEL_SIZE, std::size_t N, std::size_t S>
Image<S> copy_slice(const Image<N>& image, const std::array<int, S>& map, std::array<int, N>& coordinates)
{
        static_assert(PIXEL_SIZE > 0 && S > 0 && S < N);

        if (const std::size_t byte_count = multiply_all<long long>(image.size) * PIXEL_SIZE;
            byte_count != image.pixels.size())
        {
                error("Image byte count " + to_string(image.pixels.size()) + " is not equal to "
                      + to_string(byte_count));
        }

        Image<S> slice;

        slice.color_format = image.color_format;

        std::size_t pixel_count = 1;
        for (std::size_t i = 0; i < S; ++i)
        {
                slice.size[i] = image.size[map[i]];
                pixel_count *= slice.size[i];
        }
        slice.pixels.resize(pixel_count * PIXEL_SIZE);

        std::size_t dst_offset = 0;
        copy_slice<S - 1, PIXEL_SIZE>(
                slice.size, map, dst_offset, slice.pixels, GlobalIndex<N, long long>(image.size), image.pixels,
                coordinates);
        ASSERT(dst_offset == slice.pixels.size());

        return slice;
}
}

template <std::size_t N, std::size_t S>
Image<N - S> slice(const Image<N>& image, const std::array<Slice, S>& slices)
{
        static_assert(S > 0 && S < N);

        check_image_and_slices(image, slices);

        const std::array<int, N - S> map = create_coordinate_map<N>(slices);

        std::array<int, N> coordinates;
        for (const Slice& slice : slices)
        {
                coordinates[slice.dimension] = slice.coordinate;
        }

        switch (image.color_format)
        {
        case ColorFormat::R8_SRGB:
                return copy_slice<1>(image, map, coordinates);
        case ColorFormat::R8G8B8_SRGB:
                return copy_slice<3>(image, map, coordinates);
        case ColorFormat::R8G8B8A8_SRGB:
        case ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
                return copy_slice<4>(image, map, coordinates);
        case ColorFormat::R16:
                return copy_slice<2>(image, map, coordinates);
        case ColorFormat::R16G16B16:
        case ColorFormat::R16G16B16_SRGB:
                return copy_slice<6>(image, map, coordinates);
        case ColorFormat::R16G16B16A16:
        case ColorFormat::R16G16B16A16_SRGB:
        case ColorFormat::R16G16B16A16_PREMULTIPLIED:
                return copy_slice<8>(image, map, coordinates);
        case ColorFormat::R32:
                return copy_slice<4>(image, map, coordinates);
        case ColorFormat::R32G32B32:
                return copy_slice<12>(image, map, coordinates);
        case ColorFormat::R32G32B32A32:
        case ColorFormat::R32G32B32A32_PREMULTIPLIED:
                return copy_slice<16>(image, map, coordinates);
        }
        unknown_color_format_error(image.color_format);
}

#define TEMPLATE(N, M) template Image<(N) - (M)> slice(const Image<(N)>&, const std::array<Slice, M>&);

TEMPLATE_INSTANTIATION_N_M(TEMPLATE)
}
