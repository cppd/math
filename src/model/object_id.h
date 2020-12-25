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

#include <cstdint>
#include <functional>

namespace ns
{
class ObjectId final
{
        using Type = std::uint_least32_t;

        Type m_id;

public:
        using T = Type;

        ObjectId();

        bool operator==(const ObjectId& id) const noexcept
        {
                return m_id == id.m_id;
        }

        std::size_t hash() const noexcept
        {
                return std::hash<Type>()(m_id);
        }
};
}

namespace std
{
template <>
struct hash<::ns::ObjectId>
{
        std::size_t operator()(const ::ns::ObjectId& id) const noexcept
        {
                return id.hash();
        }
};
}
