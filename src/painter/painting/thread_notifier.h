/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "../painter.h"

#include <array>
#include <cstddef>

namespace ns::painter::painting
{
template <std::size_t N>
class ThreadNotifier final
{
        Notifier<N>* notifier_;
        unsigned thread_;

public:
        ThreadNotifier(Notifier<N>* const notifier, const unsigned thread, const std::array<int, N>& pixel)
                : notifier_(notifier),
                  thread_(thread)
        {
                notifier_->thread_busy(thread_, pixel);
        }

        ~ThreadNotifier()
        {
                notifier_->thread_free(thread_);
        }

        ThreadNotifier(const ThreadNotifier&) = delete;
        ThreadNotifier& operator=(const ThreadNotifier&) = delete;
        ThreadNotifier(ThreadNotifier&&) = delete;
        ThreadNotifier& operator=(ThreadNotifier&&) = delete;
};
}
