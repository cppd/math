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

#include "thread_queue.h"
#include "view.h"

#include <src/view/event.h>

#include <vector>

namespace ns::view::com
{
class ThreadEvents final
{
        class ReceiveInfo;

        ThreadQueue<Command> send_queue_;
        ThreadQueue<ReceiveInfo*> receive_queue_;

public:
        explicit ThreadEvents(std::vector<Command>&& commands);

        void send(Command&& command);
        void receive(const std::vector<Info>& info);

        void dispatch(View* view);
        void dispatch();
};
}
