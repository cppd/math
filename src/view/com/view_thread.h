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

#pragma once

#include "../interface.h"

#include <src/com/error.h>
#include <src/com/message.h>
#include <src/com/thread.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace ns::view
{
namespace implementation
{
template <typename T>
class ThreadQueue final
{
        SpinLock m_lock;
        std::queue<T> m_queue;

public:
        template <typename A>
        void push(A&& e)
        {
                std::lock_guard lg(m_lock);
                m_queue.push(std::forward<A>(e));
        }

        std::vector<T> pop()
        {
                static_assert(std::is_nothrow_move_assignable_v<T>);
                static_assert(std::is_nothrow_destructible_v<T>);

                std::vector<T> result;
                {
                        std::lock_guard lg(m_lock);
                        while (!m_queue.empty())
                        {
                                result.push_back(std::move(m_queue.front()));
                                m_queue.pop();
                        }
                }
                return result;
        }
};

template <typename T>
class EventQueues final
{
        ThreadQueue<Command> m_send_queue;

        struct ViewInfoExt
        {
                const std::vector<Info>* info;

                std::mutex mutex;
                std::condition_variable cv;
                bool received;
        };
        ThreadQueue<ViewInfoExt*> m_receive_queue;

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
                m_send_queue.push(std::move(event));
        }

        void receive(const std::vector<Info>& info)
        {
                ViewInfoExt v;
                v.info = &info;
                v.received = false;
                m_receive_queue.push(&v);

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
                view->send(m_send_queue.pop());

                for (ViewInfoExt* event : m_receive_queue.pop())
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
        const std::thread::id m_thread_id = std::this_thread::get_id();
        implementation::EventQueues<T> m_event_queues;
        std::thread m_thread;
        std::atomic_bool m_stop{false};
        std::atomic_bool m_started{false};

        void send(Command&& event) override
        {
                m_event_queues.send(std::move(event));
        }

        void receive(const std::vector<Info>& info) override
        {
                m_event_queues.receive(info);
        }

        void thread_function(window::WindowID parent_window, double parent_window_ppi)
        {
                try
                {
                        const std::thread::id thread_id = std::this_thread::get_id();

                        T view(parent_window, parent_window_ppi);

                        m_started = true;

                        try
                        {
                                view.loop(
                                        [&]()
                                        {
                                                ASSERT(std::this_thread::get_id() == thread_id);
                                                m_event_queues.dispatch_events(&view);
                                        },
                                        &m_stop);

                                if (!m_stop)
                                {
                                        error("Thread ended without stop.");
                                }
                        }
                        catch (const std::exception& e)
                        {
                                MESSAGE_ERROR_FATAL(std::string("Error from view\n") + e.what());
                        }
                        catch (...)
                        {
                                MESSAGE_ERROR_FATAL("Unknown error from view");
                        }
                }
                catch (const std::exception& e)
                {
                        m_started = true;
                        MESSAGE_ERROR_FATAL(std::string("Error from view\n") + e.what());
                }
                catch (...)
                {
                        m_started = true;
                        MESSAGE_ERROR_FATAL("Unknown error from view");
                }
        }

        void join_thread()
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (m_thread.joinable())
                {
                        m_stop = true;
                        m_thread.join();
                }
        }

public:
        ViewThread(window::WindowID parent_window, double parent_window_ppi, std::vector<Command>&& initial_commands)
                : m_event_queues(std::move(initial_commands))
        {
                try
                {
                        m_thread = std::thread(
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
                        } while (!m_started);
                }
                catch (...)
                {
                        join_thread();
                        throw;
                }
        }

        ~ViewThread() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                join_thread();
        }
};
}
