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

#include "main_thread.h"

#include <src/com/error.h>

namespace gui::application
{
namespace
{
const ThreadQueue* global_thread_queue = nullptr;
}

MainThreadQueue::MainThreadQueue()
{
        ASSERT(!global_thread_queue);
        global_thread_queue = &m_thread_queue;
}

MainThreadQueue::~MainThreadQueue()
{
        ASSERT(global_thread_queue);
        global_thread_queue = nullptr;
}

void MainThreadQueue::push(const std::function<void()>& f)
{
        ASSERT(global_thread_queue);
        global_thread_queue->push(f);
}
}
