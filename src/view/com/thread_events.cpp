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
#include "view.h"

#include <src/view/event.h>

#include <condition_variable>
#include <mutex>
#include <utility>
#include <vector>

namespace ns::view::com
{
class ThreadEvents::ReceiveInfo final
{
        const std::vector<Info>* info_;

        std::mutex mutex_;
        std::condition_variable cv_;
        bool received_ = false;

public:
        explicit ReceiveInfo(const std::vector<Info>* const info)
                : info_(info)
        {
        }

        [[nodiscard]] const std::vector<Info>& info() const
        {
                return *info_;
        }

        void wait()
        {
                std::unique_lock lock(mutex_);
                cv_.wait(
                        lock,
                        [&]
                        {
                                return received_;
                        });
        }

        void notify()
        {
                {
                        const std::lock_guard<std::mutex> lock(mutex_);
                        received_ = true;
                }
                cv_.notify_all();
        }
};

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
        ReceiveInfo v(&info);
        receive_queue_.push(&v);
        v.wait();
}

void ThreadEvents::dispatch(View* const view)
{
        view->exec(send_queue_.pop());

        for (ReceiveInfo* const info : receive_queue_.pop())
        {
                view->receive(info->info());
                info->notify();
        }
}

void ThreadEvents::dispatch()
{
        for (ReceiveInfo* const info : receive_queue_.pop())
        {
                info->notify();
        }
}
}
