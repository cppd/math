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

#pragma once

#include <src/model/object_id.h>

#include <variant>

namespace storage
{
struct Event
{
        struct LoadedMeshObject final
        {
                ObjectId id;
                size_t dimension;
                LoadedMeshObject(ObjectId id, size_t dimension) : id(id), dimension(dimension)
                {
                }
        };

        struct LoadedPainterMeshObject final
        {
                ObjectId id;
                size_t dimension;
                explicit LoadedPainterMeshObject(ObjectId id, size_t dimension) : id(id), dimension(dimension)
                {
                }
        };

        struct LoadedVolumeObject final
        {
                ObjectId id;
                size_t dimension;
                LoadedVolumeObject(ObjectId id, size_t dimension) : id(id), dimension(dimension)
                {
                }
        };

        struct DeletedObject final
        {
                ObjectId id;
                size_t dimension;
                DeletedObject(ObjectId id, size_t dimension) : id(id), dimension(dimension)
                {
                }
        };

        struct DeletedAll final
        {
                size_t dimension;
                explicit DeletedAll(size_t dimension) : dimension(dimension)
                {
                }
        };

        using T =
                std::variant<DeletedAll, DeletedObject, LoadedMeshObject, LoadedPainterMeshObject, LoadedVolumeObject>;

        template <typename Type>
        Event(Type&& arg) : m_data(std::forward<Type>(arg))
        {
        }

        const T& data() const
        {
                return m_data;
        }

private:
        T m_data;
};
}
