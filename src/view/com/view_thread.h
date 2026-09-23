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

#include "thread_events.h"
#include "view.h"

#include <src/view/event.h>
#include <src/view/view.h>

#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

namespace ns::view::com
{
class ViewThread final : public view::View
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        ThreadEvents thread_events_;
        std::thread thread_;
        std::atomic_bool stop_{false};
        std::atomic_bool started_{false};

        void send(Command&& event) override;
        void receive(const std::vector<Info>& info) override;

        void thread_function(const std::function<std::unique_ptr<com::View>()>& constructor);
        void join_thread();

public:
        ViewThread(const ViewThread&) = delete;
        ViewThread(ViewThread&&) = delete;
        ViewThread& operator=(const ViewThread&) = delete;
        ViewThread& operator=(ViewThread&&) = delete;

        ViewThread(std::vector<Command>&& initial_commands, std::function<std::unique_ptr<com::View>()>&& constructor);

        ~ViewThread() override;
};
}
