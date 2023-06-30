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

#include "volumes.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/model/volume_utility.h>
#include <src/settings/instantiation.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>

namespace ns::storage
{
namespace
{
constexpr unsigned MAXIMUM_VOLUME_SIZE = 1'000'000'000;

template <std::size_t N>
void check_volume_size(const unsigned size)
{
        if (size < 2)
        {
                error("Volume size is too small");
        }

        const double volume_size = std::pow(size, N);

        if (volume_size > MAXIMUM_VOLUME_SIZE)
        {
                error("Volume size is too large (" + to_string(volume_size) + "), maximum volume size is "
                      + to_string(MAXIMUM_VOLUME_SIZE));
        }
}

template <std::size_t N, std::size_t LEVEL, typename F>
void image_coordinates(const std::array<int, N>& size, Vector<N, float>* const coordinates, const F& f)
{
        static_assert(LEVEL < N);

        constexpr std::size_t D = N - LEVEL - 1;

        ASSERT(size[D] > 0);
        ASSERT(size[D] <= 1e6);

        const float max_i = size[D] - 1;

        for (int i = 0; i < size[D]; ++i)
        {
                (*coordinates)[D] = i / max_i;

                if constexpr (LEVEL + 1 < N)
                {
                        image_coordinates<N, LEVEL + 1>(size, coordinates, f);
                }
                else
                {
                        f(*coordinates);
                }
        }
}

template <std::size_t N, typename F>
void image_coordinates(const std::array<int, N>& size, const F& f)
{
        Vector<N, float> coordinates;
        image_coordinates<N, 0>(size, &coordinates, f);
}

template <std::size_t N>
model::volume::Volume<N> create_volume(const std::array<int, N>& size, const image::ColorFormat color_format)
{
        model::volume::Volume<N> volume;

        volume.image.size = size;
        volume.image.color_format = color_format;
        volume.image.pixels.resize(
                image::format_pixel_size_in_bytes(color_format) * multiply_all<long long>(volume.image.size));
        volume.matrix = model::volume::matrix_for_image_size(size);

        return volume;
}

template <typename I, typename F>
I float_to_uint(const F v)
{
        static_assert(std::is_same_v<F, float>);
        static_assert(std::is_same_v<I, std::uint8_t> || std::is_same_v<I, std::uint16_t>);
        return v * F{Limits<I>::max()} + F{0.5};
}

template <std::size_t N>
std::unique_ptr<model::volume::Volume<N>> scalar_cube(const unsigned size)
{
        static constexpr image::ColorFormat COLOR_FORMAT = image::ColorFormat::R16;
        using Type = std::uint16_t;

        static_assert(format_pixel_size_in_bytes(COLOR_FORMAT) == sizeof(Type));

        static constexpr Type VALUE = 10000;
        static constexpr Type MIN = 500;

        check_volume_size<N>(size);

        std::array<int, N> sizes;
        for (unsigned i = 0; i < N; ++i)
        {
                sizes[i] = size;
        }

        model::volume::Volume<N> volume = create_volume(sizes, COLOR_FORMAT);

        std::byte* ptr = volume.image.pixels.data();

        const Vector<N, float> center(0.5f);
        image_coordinates<N>(
                volume.image.size,
                [&](const Vector<N, float>& coordinates)
                {
                        Vector<N, float> p = coordinates - center;
                        bool cube = true;
                        for (unsigned i = 0; i < N; ++i)
                        {
                                if (std::abs(p[i]) > 0.4f)
                                {
                                        cube = false;
                                        break;
                                }
                        }
                        Type value = cube ? VALUE : MIN;
                        std::memcpy(ptr, &value, sizeof(value));
                        ptr += sizeof(value);
                });

        ASSERT(ptr = volume.image.pixels.data() + volume.image.pixels.size());

        return std::make_unique<model::volume::Volume<N>>(std::move(volume));
}

template <std::size_t N>
std::unique_ptr<model::volume::Volume<N>> scalar_ellipsoid(const unsigned size)
{
        constexpr image::ColorFormat COLOR_FORMAT = image::ColorFormat::R16;
        using Type = std::uint16_t;

        static_assert(format_pixel_size_in_bytes(COLOR_FORMAT) == sizeof(Type));

        constexpr Type MIN = 500;

        check_volume_size<N>(size);

        if (size / 2 < 2)
        {
                error("Ellipsiod size is too small");
        }

        std::array<int, N> sizes;
        sizes[0] = size;
        for (unsigned i = 1; i < N; ++i)
        {
                sizes[i] = size / 2;
        }

        model::volume::Volume<N> volume = create_volume(sizes, COLOR_FORMAT);

        std::byte* ptr = volume.image.pixels.data();

        const Vector<N, float> center(0.5f);
        image_coordinates<N>(
                volume.image.size,
                [&](const Vector<N, float>& coordinates)
                {
                        const Vector<N, float> p = coordinates - center;
                        const float distance = 2 * p.norm();
                        Type value = std::max(MIN, float_to_uint<Type>(1.0f - std::clamp(distance, 0.0f, 1.0f)));
                        std::memcpy(ptr, &value, sizeof(value));
                        ptr += sizeof(value);
                });

        ASSERT(ptr = volume.image.pixels.data() + volume.image.pixels.size());

        return std::make_unique<model::volume::Volume<N>>(std::move(volume));
}

template <std::size_t N>
std::unique_ptr<model::volume::Volume<N>> color_cube(const unsigned size)
{
        static_assert(N >= 3);

        constexpr image::ColorFormat COLOR_FORMAT = image::ColorFormat::R8G8B8A8_SRGB;

        check_volume_size<N>(size);

        std::array<int, N> sizes;
        for (unsigned i = 0; i < N; ++i)
        {
                sizes[i] = size;
        }

        model::volume::Volume<N> volume = create_volume(sizes, COLOR_FORMAT);

        std::array<std::uint8_t, 4> color;
        const std::uint8_t alpha = std::max(std::uint8_t{1}, float_to_uint<std::uint8_t>(1.0f / size));
        color[3] = alpha;

        std::byte* ptr = volume.image.pixels.data();

        image_coordinates<N>(
                volume.image.size,
                [&](const Vector<N, float>& coordinates)
                {
                        for (std::size_t i = 0; i < N; ++i)
                        {
                                const float c = coordinates[i] / (1 << (i / 3));
                                color[i % 3] = float_to_uint<std::uint8_t>(c);
                        }
                        std::memcpy(ptr, color.data(), color.size());
                        ptr += color.size();
                });

        ASSERT(ptr = volume.image.pixels.data() + volume.image.pixels.size());

        return std::make_unique<model::volume::Volume<N>>(std::move(volume));
}

template <typename T>
std::vector<std::string> names_of_map(const std::map<std::string, T>& map)
{
        std::vector<std::string> res;
        res.reserve(map.size());
        for (const auto& e : map)
        {
                res.push_back(e.first);
        }
        return res;
}

template <std::size_t N>
class Impl final : public VolumeObjectRepository<N>
{
        std::map<std::string, std::function<std::unique_ptr<model::volume::Volume<N>>(unsigned)>> map_;

        [[nodiscard]] std::vector<std::string> object_names() const override
        {
                return names_of_map(map_);
        }

        [[nodiscard]] std::unique_ptr<model::volume::Volume<N>> object(
                const std::string& object_name,
                const unsigned size) const override
        {
                const auto iter = map_.find(object_name);
                if (iter != map_.cend())
                {
                        return iter->second(size);
                }
                error("Object not found in repository: " + object_name);
        }

public:
        Impl()
        {
                map_.emplace("Scalar Cube", scalar_cube<N>);
                map_.emplace("Scalar Ellipsoid", scalar_ellipsoid<N>);
                map_.emplace("Color Cube", color_cube<N>);
        }
};
}

template <std::size_t N>
std::unique_ptr<VolumeObjectRepository<N>> create_volume_object_repository()
{
        return std::make_unique<Impl<N>>();
}

#define TEMPLATE(N) template std::unique_ptr<VolumeObjectRepository<(N)>> create_volume_object_repository();

TEMPLATE_INSTANTIATION_N(TEMPLATE)
}
