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

#include <cmath>
#include <cstring>
#include <functional>
#include <map>

namespace storage
{
namespace
{
constexpr unsigned MAXIMUM_VOLUME_SIZE = 1'000'000'000;

template <size_t N>
double volume_size(unsigned size)
{
        return std::pow(size, N);
}

template <typename T>
uint8_t float_to_uint8(T v)
{
        return v * T(255) + T(0.5);
}

template <size_t N>
std::unique_ptr<volume::Volume<N>> scalar_cube(unsigned size)
{
        constexpr image::ColorFormat COLOR_FORMAT = image::ColorFormat::R16;
        constexpr std::uint16_t VALUE = 1000;

        if (size < 2)
        {
                error("Volume size is too small");
        }

        if (volume_size<N>(size) > MAXIMUM_VOLUME_SIZE)
        {
                error("Volume size is too large (" + to_string(volume_size<N>(size)) + "), maximum volume size is "
                      + to_string(MAXIMUM_VOLUME_SIZE));
        }

        volume::Volume<N> volume;

        volume.matrix = Matrix<N + 1, N + 1, double>(1);

        for (unsigned i = 0; i < N; ++i)
        {
                volume.image.size[i] = size;
        }
        volume.image.color_format = COLOR_FORMAT;
        volume.image.pixels.resize(sizeof(VALUE) * multiply_all<long long>(volume.image.size));

        auto iter = volume.image.pixels.begin();
        while (iter != volume.image.pixels.end())
        {
                std::memcpy(&(*iter), &VALUE, sizeof(VALUE));
                std::advance(iter, sizeof(VALUE));
        }

        return std::make_unique<volume::Volume<N>>(std::move(volume));
}

template <size_t N, size_t Level>
void cube_pixels(uint8_t alpha, float size, float max_i_reciprocal, Vector<N, float>* coordinates, std::byte** pixels)
{
        static_assert(Level < N);

        for (float i = 0; i < size; ++i)
        {
                (*coordinates)[Level] = i * max_i_reciprocal;

                if constexpr (Level + 1 < N)
                {
                        cube_pixels<N, Level + 1>(alpha, size, max_i_reciprocal, coordinates, pixels);
                }
                else
                {
                        std::array<std::uint8_t, 4> color;
                        for (size_t n = 0; n < N; ++n)
                        {
                                float c = (*coordinates)[n] / (1 << (n / 3));
                                color[n % 3] = float_to_uint8(c);
                        }
                        color[3] = alpha;
                        std::memcpy(*pixels, color.data(), 4);
                        *pixels += 4;
                }
        }
}

template <size_t N>
std::unique_ptr<volume::Volume<N>> color_cube(unsigned size)
{
        constexpr image::ColorFormat COLOR_FORMAT = image::ColorFormat::R8G8B8A8_SRGB;

        if (size < 2)
        {
                error("Volume size is too small");
        }

        if (volume_size<N>(size) > MAXIMUM_VOLUME_SIZE)
        {
                error("Volume size is too large (" + to_string(volume_size<3>(size)) + "), maximum volume size is "
                      + to_string(MAXIMUM_VOLUME_SIZE));
        }

        volume::Volume<N> volume;

        volume.matrix = Matrix<N + 1, N + 1, double>(1);

        for (unsigned i = 0; i < N; ++i)
        {
                volume.image.size[i] = size;
        }

        volume.image.color_format = COLOR_FORMAT;
        volume.image.pixels.resize(
                image::format_pixel_size_in_bytes(COLOR_FORMAT) * multiply_all<long long>(volume.image.size));

        uint8_t alpha = std::max(uint8_t(1), float_to_uint8(1.0f / size));
        Vector<N, float> point_coordinates;
        std::byte* ptr = volume.image.pixels.data();

        cube_pixels<N, 0>(alpha, size, 1.0f / (size - 1), &point_coordinates, &ptr);

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
