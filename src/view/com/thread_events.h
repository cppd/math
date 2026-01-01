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

#include <src/view/event.h>

#include <condition_variable>
#include <mutex>
#include <utility>
#include <vector>

namespace ns::view::com
{
namespace thread_events_implementation
{
class ReceiveInfo final
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
}

template <typename T>
class ThreadEvents final
{
        ThreadQueue<Command> send_queue_;
        ThreadQueue<thread_events_implementation::ReceiveInfo*> receive_queue_;

public:
        explicit ThreadEvents(std::vector<Command>&& commands)
        {
                for (Command& command : commands)
                {
                        send(std::move(command));
                }
        }

        void send(Command&& command)
        {
                send_queue_.push(std::move(command));
        }

        void receive(const std::vector<Info>& info)
        {
                namespace impl = thread_events_implementation;

                impl::ReceiveInfo v(&info);
                receive_queue_.push(&v);
                v.wait();
        }

        void dispatch(T* const view)
        {
                namespace impl = thread_events_implementation;

                view->exec(send_queue_.pop());

                for (impl::ReceiveInfo* const info : receive_queue_.pop())
                {
                        view->receive(info->info());
                        info->notify();
                }
        }

        void dispatch()
        {
                namespace impl = thread_events_implementation;

                for (impl::ReceiveInfo* const info : receive_queue_.pop())
                {
                        info->notify();
                }
        }
};
}
