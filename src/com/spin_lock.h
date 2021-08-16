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
class SpinLock final
{
        std::atomic_flag spin_lock_;

public:
        SpinLock() noexcept
        {
                spin_lock_.clear();
        }

        void lock() noexcept
        {
                while (spin_lock_.test_and_set(std::memory_order_acquire))
                {
                }
        }

        void unlock() noexcept
        {
                spin_lock_.clear(std::memory_order_release);
        }
};
}
