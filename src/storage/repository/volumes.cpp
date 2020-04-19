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

template <size_t N>
std::unique_ptr<volume::Volume<N>> cube(unsigned size)
{
        constexpr unsigned PIXEL_VALUE = 1000;

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
        volume.image.pixels.resize(multiply_all<long long>(volume.image.size), PIXEL_VALUE);

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
                m_map.emplace("Cube", cube<N>);
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
