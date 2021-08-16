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

#pragma once

#include <atomic>

namespace ns
{
template <typename T>
class AtomicCounter
{
        static_assert(std::atomic<T>::is_always_lock_free);

        std::atomic<T> counter_;

public:
        static constexpr bool is_always_lock_free = std::atomic<T>::is_always_lock_free;

        AtomicCounter() : counter_(0)
        {
        }

        explicit AtomicCounter(std::type_identity_t<T> v) : counter_(v)
        {
        }

        AtomicCounter& operator=(T v)
        {
                counter_.store(v, std::memory_order_relaxed);
                return *this;
        }

        operator T() const
        {
                return counter_.load(std::memory_order_relaxed);
        }

        void operator++()
        {
                counter_.fetch_add(1, std::memory_order_relaxed);
        }

        void operator+=(T v)
        {
                counter_.fetch_add(v, std::memory_order_relaxed);
        }
};
}
