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

#include "thread_events.h"

#include "thread_queue.h"
#include "thread_receive.h"
#include "view.h"

#include <src/view/event.h>

#include <utility>
#include <vector>

namespace ns::view::com
{
ThreadEvents::ThreadEvents(std::vector<Command>&& commands)
{
        for (Command& command : commands)
        {
                send(std::move(command));
        }
}

void ThreadEvents::send(Command&& command)
{
        send_queue_.push(std::move(command));
}

void ThreadEvents::receive(const std::vector<Info>& info)
{
        ThreadReceive v(&info);
        receive_queue_.push(&v);
        v.wait();
}

void ThreadEvents::dispatch(View* const view)
{
        view->exec(send_queue_.pop());

        for (ThreadReceive<const std::vector<Info>*>* const info : receive_queue_.pop())
        {
                view->receive(*info->info());
                info->notify();
        }
}

void ThreadEvents::dispatch()
{
        for (ThreadReceive<const std::vector<Info>*>* const info : receive_queue_.pop())
        {
                info->notify();
        }
}
}
