/*
Copyright (C) 2017-2026 Topological Manifold

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
class Spinlock final
{
        std::atomic_flag flag_;

public:
        Spinlock() noexcept
        {
                flag_.clear(std::memory_order_relaxed);
        }

        void lock() noexcept
        {
                while (flag_.test_and_set(std::memory_order_acquire))
                {
                        while (flag_.test(std::memory_order_relaxed))
                        {
                        }
                }
        }

        void unlock() noexcept
        {
                flag_.clear(std::memory_order_release);
        }
};
}
