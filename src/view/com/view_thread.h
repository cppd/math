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

#include "../interface.h"

#include <src/com/error.h>
#include <src/com/message.h>
#include <src/com/spin_lock.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

namespace ns::view
{
namespace implementation
{
template <typename T>
class ThreadQueue final
{
        SpinLock lock_;
        std::queue<T> queue_;

public:
        template <typename A>
        void push(A&& e)
        {
                std::lock_guard lg(lock_);
                queue_.push(std::forward<A>(e));
        }

        std::vector<T> pop()
        {
                static_assert(std::is_nothrow_move_assignable_v<T>);
                static_assert(std::is_nothrow_destructible_v<T>);

                std::vector<T> result;
                {
                        std::lock_guard lg(lock_);
                        while (!queue_.empty())
                        {
                                result.push_back(std::move(queue_.front()));
                                queue_.pop();
                        }
                }
                return result;
        }
};

template <typename T>
class EventQueues final
{
        ThreadQueue<Command> send_queue_;

        struct ViewInfoExt
        {
                const std::vector<Info>* info;

                std::mutex mutex;
                std::condition_variable cv;
                bool received;
        };
        ThreadQueue<ViewInfoExt*> receive_queue_;

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
                ViewInfoExt v;
                v.info = &info;
                v.received = false;
                receive_queue_.push(&v);

                std::unique_lock<std::mutex> lock(v.mutex);
                v.cv.wait(
                        lock,
                        [&]
                        {
                                return v.received;
                        });
        }

        void dispatch_events(T* view)
        {
                view->send(send_queue_.pop());

                for (ViewInfoExt* event : receive_queue_.pop())
                {
                        view->receive(*(event->info));
                        {
                                std::lock_guard<std::mutex> lock(event->mutex);
                                event->received = true;
                        }
                        event->cv.notify_all();
                }
        }
};
}

template <typename T>
class ViewThread final : public View
{
        const std::thread::id thread_id_ = std::this_thread::get_id();
        implementation::EventQueues<T> event_queues_;
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

        void thread_function(window::WindowID parent_window, double parent_window_ppi)
        {
                try
                {
                        [[maybe_unused]] const std::thread::id thread_id = std::this_thread::get_id();

                        T view(parent_window, parent_window_ppi);

                        started_ = true;

                        try
                        {
                                view.loop(
                                        [&]()
                                        {
                                                ASSERT(std::this_thread::get_id() == thread_id);
                                                event_queues_.dispatch_events(&view);
                                        },
                                        &stop_);

                                if (!stop_)
                                {
                                        error("Thread ended without stop.");
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
        ViewThread(window::WindowID parent_window, double parent_window_ppi, std::vector<Command>&& initial_commands)
                : event_queues_(std::move(initial_commands))
        {
                try
                {
                        thread_ = std::thread(
                                [=, this]()
                                {
                                        try
                                        {
                                                thread_function(parent_window, parent_window_ppi);
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
