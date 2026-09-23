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

#include "view_thread.h"

#include "thread_events.h"
#include "view.h"

#include <src/com/error.h>
#include <src/com/message.h>
#include <src/view/event.h>

#include <atomic>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace ns::view::com
{
void ViewThread::send(Command&& event)
{
        thread_events_.send(std::move(event));
}

void ViewThread::receive(const std::vector<Info>& info)
{
        thread_events_.receive(info);
}

void ViewThread::thread_function(const std::function<std::unique_ptr<com::View>()>& constructor)
{
        try
        {
                const std::unique_ptr<com::View> view = constructor();
                ASSERT(view);

                started_ = true;
                try
                {
                        while (!stop_)
                        {
                                thread_events_.dispatch(view.get());
                                view->render();
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

void ViewThread::join_thread()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        if (thread_.joinable())
        {
                stop_ = true;
                thread_.join();
        }
}

ViewThread::ViewThread(
        std::vector<Command>&& initial_commands,
        std::function<std::unique_ptr<com::View>()>&& constructor)
        : thread_events_(std::move(initial_commands))
{
        try
        {
                thread_ = std::thread(
                        [constructor = std::move(constructor), this]
                        {
                                try
                                {
                                        thread_function(constructor);
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

ViewThread::~ViewThread()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        join_thread();
}
}
