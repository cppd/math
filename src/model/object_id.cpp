/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "object_id.h"

#include <atomic>

namespace ns
{
namespace
{
static_assert(std::is_unsigned_v<ObjectId::T>);
using AtomicType = std::atomic<ObjectId::T>;
static_assert(AtomicType::is_always_lock_free);
AtomicType g_current_id = 0;
}

ObjectId::ObjectId()
{
        m_id = 1 + g_current_id.fetch_add(1, std::memory_order_relaxed);
}
}
