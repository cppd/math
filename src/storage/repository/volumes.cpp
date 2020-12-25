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

#include "volumes.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/model/volume_utility.h>

#include <cmath>
#include <cstring>
#include <functional>
#include <map>

namespace ns::storage
{
namespace
{
constexpr unsigned MAXIMUM_VOLUME_SIZE = 1'000'000'000;

template <size_t N>
void check_volume_size(unsigned size)
{
        if (size < 2)
        {
                error("Volume size is too small");
        }

        double volume_size = std::pow(size, N);

        if (volume_size > MAXIMUM_VOLUME_SIZE)
        {
                error("Volume size is too large (" + to_string(volume_size) + "), maximum volume size is "
                      + to_string(MAXIMUM_VOLUME_SIZE));
        }
}

template <size_t N, size_t Level, typename F>
void image_coordinates(const std::array<int, N>& size, Vector<N, float>* coordinates, const F& f)
{
        static_assert(Level < N);

        constexpr size_t D = N - Level - 1;

        ASSERT(size[D] > 0);
        ASSERT(size[D] <= 1e6);

        float max_i = size[D] - 1;
        float max_i_reciprocal = 1.0f / max_i;

        for (float i = 0; i <= max_i; ++i)
        {
                (*coordinates)[D] = i * max_i_reciprocal;

                if constexpr (Level + 1 < N)
                {
                        image_coordinates<N, Level + 1>(size, coordinates, f);
                }
                else
                {
                        f(*coordinates);
                }
        }
}

template <size_t N, typename F>
void image_coordinates(const std::array<int, N>& size, const F& f)
{
        Vector<N, float> coordinates;
        image_coordinates<N, 0>(size, &coordinates, f);
}

template <size_t N>
void init_volume(const std::array<int, N>& size, image::ColorFormat color_format, volume::Volume<N>* volume)
{
        volume->image.size = size;
        volume->image.color_format = color_format;
        volume->image.pixels.resize(
                image::format_pixel_size_in_bytes(color_format) * multiply_all<long long>(volume->image.size));
        volume->matrix = volume::matrix_for_image_size(size);
}

template <typename I, typename F>
I float_to_uint(F v)
{
        static_assert(std::is_same_v<F, float>);
        static_assert(std::is_same_v<I, uint8_t> || std::is_same_v<I, uint16_t>);
        return v * F(limits<I>::max()) + F(0.5);
}

template <size_t N>
std::unique_ptr<volume::Volume<N>> scalar_cube(unsigned size)
{
        constexpr image::ColorFormat COLOR_FORMAT = image::ColorFormat::R16;
        using DATA_TYPE = uint16_t;

        constexpr DATA_TYPE VALUE = 10000;
        constexpr DATA_TYPE MIN = 500;

        check_volume_size<N>(size);

        volume::Volume<N> volume;

        std::array<int, N> sizes;
        for (unsigned i = 0; i < N; ++i)
        {
                sizes[i] = size;
        }
        init_volume(sizes, COLOR_FORMAT, &volume);

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
                        DATA_TYPE value = cube ? VALUE : MIN;
                        std::memcpy(ptr, &value, sizeof(value));
                        ptr += sizeof(value);
                });

        ASSERT(ptr = volume.image.pixels.data() + volume.image.pixels.size());

        return std::make_unique<volume::Volume<N>>(std::move(volume));
}

template <size_t N>
std::unique_ptr<volume::Volume<N>> scalar_ellipsoid(unsigned size)
{
        constexpr image::ColorFormat COLOR_FORMAT = image::ColorFormat::R16;
        using DATA_TYPE = uint16_t;

        constexpr DATA_TYPE MIN = 500;

        check_volume_size<N>(size);

        if (size / 2 < 2)
        {
                error("Ellipsiod size is too small");
        }

        volume::Volume<N> volume;

        std::array<int, N> sizes;
        sizes[0] = size;
        for (unsigned i = 1; i < N; ++i)
        {
                sizes[i] = size / 2;
        }
        init_volume(sizes, COLOR_FORMAT, &volume);

        std::byte* ptr = volume.image.pixels.data();

        const Vector<N, float> center(0.5f);
        image_coordinates<N>(
                volume.image.size,
                [&](const Vector<N, float>& coordinates)
                {
                        Vector<N, float> p = coordinates - center;
                        float distance = 2 * p.norm();
                        DATA_TYPE value =
                                std::max(MIN, float_to_uint<DATA_TYPE>(1.0f - std::clamp(distance, 0.0f, 1.0f)));
                        std::memcpy(ptr, &value, sizeof(value));
                        ptr += sizeof(value);
                });

        ASSERT(ptr = volume.image.pixels.data() + volume.image.pixels.size());

        return std::make_unique<volume::Volume<N>>(std::move(volume));
}

template <size_t N>
std::unique_ptr<volume::Volume<N>> color_cube(unsigned size)
{
        static_assert(N >= 3);

        constexpr image::ColorFormat COLOR_FORMAT = image::ColorFormat::R8G8B8A8_SRGB;

        check_volume_size<N>(size);

        volume::Volume<N> volume;

        std::array<int, N> sizes;
        for (unsigned i = 0; i < N; ++i)
        {
                sizes[i] = size;
        }
        init_volume(sizes, COLOR_FORMAT, &volume);

        std::array<std::uint8_t, 4> color;
        uint8_t alpha = std::max(uint8_t(1), float_to_uint<uint8_t>(1.0f / size));
        color[3] = alpha;

        std::byte* ptr = volume.image.pixels.data();

        image_coordinates<N>(
                volume.image.size,
                [&](const Vector<N, float>& coordinates)
                {
                        for (size_t n = 0; n < N; ++n)
                        {
                                float c = coordinates[n] / (1 << (n / 3));
                                color[n % 3] = float_to_uint<uint8_t>(c);
                        }
                        std::memcpy(ptr, color.data(), color.size());
                        ptr += color.size();
                });

        ASSERT(ptr = volume.image.pixels.data() + volume.image.pixels.size());

        return std::make_unique<volume::Volume<N>>(std::move(volume));
}

template <typename T>
std::vector<std::string> names_of_map(const std::map<std::string, T>& map)
{
        std::vector<std::string> names;
        names.reserve(map.size());

        for (const auto& e : map)
        {
                names.push_back(e.first);
        }

        return names;
}

template <size_t N>
class Impl final : public VolumeObjectRepository<N>
{
        std::map<std::string, std::function<std::unique_ptr<volume::Volume<N>>(unsigned)>> m_map;

        std::vector<std::string> object_names() const override
        {
                return names_of_map(m_map);
        }

        std::unique_ptr<volume::Volume<N>> object(const std::string& object_name, unsigned size) const override
        {
                auto iter = m_map.find(object_name);
                if (iter != m_map.cend())
                {
                        return iter->second(size);
                }
                error("Object not found in repository: " + object_name);
        }

public:
        Impl()
        {
                m_map.emplace("Scalar Cube", scalar_cube<N>);
                m_map.emplace("Scalar Ellipsoid", scalar_ellipsoid<N>);
                m_map.emplace("Color Cube", color_cube<N>);
        }
};
}

template <size_t N>
std::unique_ptr<VolumeObjectRepository<N>> create_volume_object_repository()
{
        return std::make_unique<Impl<N>>();
}

template std::unique_ptr<VolumeObjectRepository<3>> create_volume_object_repository();
template std::unique_ptr<VolumeObjectRepository<4>> create_volume_object_repository();
template std::unique_ptr<VolumeObjectRepository<5>> create_volume_object_repository();
}
