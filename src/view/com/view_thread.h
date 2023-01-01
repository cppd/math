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

#include "../view.h"

#include <src/com/error.h>
#include <src/com/message.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

namespace ns::view
{
namespace view_thread_implementation
{
template <typename T>
class ThreadQueue final
{
        std::mutex lock_;
        std::queue<T> queue_;

public:
        template <typename A>
        void push(A&& e)
        {
                const std::lock_guard lg(lock_);
                queue_.push(std::forward<A>(e));
        }

        std::vector<T> pop()
        {
                static_assert(std::is_nothrow_move_assignable_v<T>);
                static_assert(std::is_nothrow_destructible_v<T>);

                std::vector<T> result;
                {
                        const std::lock_guard lg(lock_);
                        while (!queue_.empty())
                        {
                                result.push_back(std::move(queue_.front()));
                                queue_.pop();
                        }
                }
                return result;
        }
};

class QueueEvent final
{
        const std::vector<Info>* info_;

        std::mutex mutex_;
        std::condition_variable cv_;
        bool received_ = false;

public:
        explicit QueueEvent(const std::vector<Info>* const info)
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

template <typename T>
class EventQueues final
{
        ThreadQueue<Command> send_queue_;
        ThreadQueue<QueueEvent*> receive_queue_;

public:
        explicit EventQueues(std::vector<Command>&& initial_events)
        {
                for (Command& event : initial_events)
                {
                        send(std::move(event));
                }
        }

        void send(Command&& event)
        {
                send_queue_.push(std::move(event));
        }

        void receive(const std::vector<Info>& info)
        {
                QueueEvent v(&info);
                receive_queue_.push(&v);
                v.wait();
        }

        void dispatch_events(T* const view)
        {
                view->exec(send_queue_.pop());

                for (QueueEvent* const event : receive_queue_.pop())
                {
                        view->receive(event->info());
                        event->notify();
                }
        }

        void dispatch_events()
        {
                for (QueueEvent* const event : receive_queue_.pop())
                {
                        event->notify();
                }
        }
};
}

template <typename T>
class ViewThread final : public View
{
        const std::thread::id thread_id_ = std::this_thread::get_id();
        view_thread_implementation::EventQueues<T> event_queues_;
        std::thread thread_;
        std::atomic_bool stop_{false};
        std::atomic_bool started_{false};

        void send(Command&& event) override
        {
                event_queues_.send(std::move(event));
        }

        void receive(const std::vector<Info>& info) override
        {
                event_queues_.receive(info);
        }

        template <typename... Args>
        void thread_function(Args&&... args)
        {
                try
                {
                        T view(std::forward<Args>(args)...);

                        started_ = true;

                        try
                        {
                                while (!stop_)
                                {
                                        event_queues_.dispatch_events(&view);
                                        view.render();
                                }
                        }
                        catch (const std::exception& e)
                        {
                                message_error_fatal(std::string("Error from view\n") + e.what());
                        }
                        catch (...)
                        {
                                message_error_fatal("Unknown error from view");
                        }
                }
                catch (const std::exception& e)
                {
                        started_ = true;
                        message_error_fatal(std::string("Error from view\n") + e.what());
                }
                catch (...)
                {
                        started_ = true;
                        message_error_fatal("Unknown error from view");
                }

                try
                {
                        while (!stop_)
                        {
                                event_queues_.dispatch_events();
                        }
                }
                catch (const std::exception& e)
                {
                        message_error_fatal(std::string("Error while dispatching events\n") + e.what());
                }
                catch (...)
                {
                        message_error_fatal("Unknown error while dispatching events");
                }
        }

        void join_thread()
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                if (thread_.joinable())
                {
                        stop_ = true;
                        thread_.join();
                }
        }

public:
        template <typename... Args>
        explicit ViewThread(std::vector<Command>&& initial_commands, Args&&... args)
                : event_queues_(std::move(initial_commands))
        {
                try
                {
                        thread_ = std::thread(
                                [=, this]()
                                {
                                        try
                                        {
                                                thread_function(std::forward<Args>(args)...);
                                        }
                                        catch (...)
                                        {
                                                error_fatal("Exception in the view thread function");
                                        }
                                });

                        do
                        {
                                std::this_thread::yield();
                        } while (!started_);
                }
                catch (...)
                {
                        join_thread();
                        throw;
                }
        }

        ~ViewThread() override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                join_thread();
        }
};
}
