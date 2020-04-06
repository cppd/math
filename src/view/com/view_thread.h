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
#include <src/com/thread.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class ViewEventQueues final
{
        ThreadQueue<ViewEvent> m_send_queue;

        struct ViewInfoExt
        {
                const std::vector<ViewInfo*>* info;

                std::mutex mutex;
                std::condition_variable cv;
                bool received;
        };
        ThreadQueue<ViewInfoExt*> m_receive_queue;

public:
        explicit ViewEventQueues(std::vector<ViewEvent>&& initial_events)
        {
                for (ViewEvent& event : initial_events)
                {
                        send(std::move(event));
                }
        }

        void send(ViewEvent&& event)
        {
                m_send_queue.push(std::move(event));
        }

        void receive(const std::vector<ViewInfo*>& info)
        {
                ViewInfoExt v;
                v.info = &info;
                v.received = false;
                m_receive_queue.push(&v);

                std::unique_lock<std::mutex> lock(v.mutex);
                v.cv.wait(lock, [&] { return v.received; });
        }

        void dispatch_events(View* view)
        {
                {
                        std::optional<ViewEvent> event;
                        while ((event = m_send_queue.pop()))
                        {
                                view->send(std::move(*event));
                        }
                }
                {
                        std::optional<ViewInfoExt*> event;
                        while ((event = m_receive_queue.pop()))
                        {
                                view->receive(*((*event)->info));

                                {
                                        std::lock_guard<std::mutex> lock((*event)->mutex);
                                        (*event)->received = true;
                                }
                                (*event)->cv.notify_all();
                        }
                }
        }
};

template <typename T>
class ViewThread final : public View
{
        const std::thread::id m_thread_id = std::this_thread::get_id();
        ViewEventQueues m_event_queues;
        std::thread m_thread;
        std::atomic_bool m_stop{false};
        std::atomic_bool m_started{false};

        void send(ViewEvent&& event) override
        {
                m_event_queues.send(std::move(event));
        }

        void receive(const std::vector<ViewInfo*>& info) override
        {
                m_event_queues.receive(info);
        }

        void thread_function(ViewEvents* events, WindowID parent_window, double parent_window_ppi)
        {
                try
                {
                        try
                        {
                                T view(events, parent_window, parent_window_ppi);

                                m_started = true;

                                view.loop([&]() { m_event_queues.dispatch_events(&view); }, &m_stop);

                                if (!m_stop)
                                {
                                        error("Thread ended without stop.");
                                }
                        }
                        catch (...)
                        {
                                m_started = true;
                                throw;
                        }
                }
                catch (ErrorSourceException& e)
                {
                        events->message_error_source(e.msg(), e.src());
                }
                catch (std::exception& e)
                {
                        events->message_error_fatal(e.what());
                }
                catch (...)
                {
                        events->message_error_fatal("Unknown Error. Thread ended.");
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
        ViewThread(
                ViewEvents* events,
                WindowID parent_window,
                double parent_window_ppi,
                std::vector<ViewEvent>&& initial_events)
                : m_event_queues(std::move(initial_events))
        {
                try
                {
                        m_thread = std::thread([=, this]() {
                                try
                                {
                                        thread_function(events, parent_window, parent_window_ppi);
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
