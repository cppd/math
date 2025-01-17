/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <src/com/error.h>
#include <src/com/message.h>
#include <src/view/event.h>
#include <src/view/view.h>

#include <atomic>
#include <exception>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace ns::view::com
{
template <typename T>
class ViewThread final : public View
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        ThreadEvents<T> thread_events_;
        std::thread thread_;
        std::atomic_bool stop_{false};
        std::atomic_bool started_{false};

        void send(Command&& event) override
        {
                thread_events_.send(std::move(event));
        }

        void receive(const std::vector<Info>& info) override
        {
                thread_events_.receive(info);
        }

        template <typename... Args>
        void thread_function(const Args&... args)
        {
                try
                {
                        T view(args...);

                        started_ = true;
                        try
                        {
                                while (!stop_)
                                {
                                        thread_events_.dispatch(&view);
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
                                thread_events_.dispatch();
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
        explicit ViewThread(std::vector<Command>&& initial_commands, const Args&... args)
                : thread_events_(std::move(initial_commands))
        {
                try
                {
                        thread_ = std::thread(
                                [args..., this]
                                {
                                        try
                                        {
                                                thread_function(args...);
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
