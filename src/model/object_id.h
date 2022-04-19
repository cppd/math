/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <atomic>
#include <cstdint>
#include <functional>

namespace ns
{
class ObjectId final
{
        using T = std::uint_least32_t;

        inline static std::atomic<T> current_id_ = 0;
        static_assert(decltype(current_id_)::is_always_lock_free);

        T id_;

public:
        ObjectId() noexcept : id_(1 + current_id_.fetch_add(1, std::memory_order_relaxed))
        {
        }

        bool operator==(const ObjectId id) const noexcept
        {
                return id_ == id.id_;
        }

        std::size_t hash() const noexcept
        {
                return std::hash<T>()(id_);
        }
};
}

namespace std
{
template <>
struct hash<::ns::ObjectId> final
{
        std::size_t operator()(const ::ns::ObjectId id) const noexcept
        {
                return id.hash();
        }
};
}
