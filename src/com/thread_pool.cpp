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

#include "thread_pool.h"

#include "error.h"
#include "exception.h"

#include <atomic>
#include <barrier>
#include <cstddef>
#include <exception>
#include <functional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace ns
{
void ThreadPool::process(const unsigned thread_num) noexcept
{
        try
        {
                try
                {
                        try
                        {
                                function_(thread_num, thread_count_);
                        }
                        catch (const TerminateQuietlyException&)
                        {
                        }
                        catch (const std::exception& e)
                        {
                                thread_errors_[thread_num].set(e.what());
                        }
                        catch (...)
                        {
                                thread_errors_[thread_num].set("Unknown error in a thread of thread pool");
                        }
                }
                catch (const std::exception& e)
                {
                        error_fatal(std::string("Exception in thread pool while working with exception: ") + e.what());
                }
        }
        catch (...)
        {
                error_fatal("Exception in thread pool while working with exception");
        }
}

void ThreadPool::thread(const unsigned thread_num) noexcept
{
        try
        {
                while (true)
                {
                        barrier_.arrive_and_wait();

                        if (exit_)
                        {
                                return;
                        }

                        process(thread_num);

                        barrier_.arrive_and_wait();
                }
        }
        catch (...)
        {
                error_fatal("Exception in thread pool while proccessing thread");
        }
}

void ThreadPool::start_and_wait() noexcept
{
        try
        {
                try
                {
                        barrier_.arrive_and_wait();
                        barrier_.arrive_and_wait();
                }
                catch (const std::exception& e)
                {
                        error_fatal(std::string("Error start and wait threads: ") + e.what());
                }
        }
        catch (...)
        {
                error_fatal("Error start and wait threads");
        }
}

void ThreadPool::clear_errors()
{
        for (ThreadError& thread_error : thread_errors_)
        {
                thread_error.clear();
        }
}

void ThreadPool::find_errors() const
{
        bool there_is_error = false;
        std::string error_message;

        for (const ThreadError& thread_error : thread_errors_)
        {
                if (thread_error.has_error())
                {
                        there_is_error = true;
                        if (!error_message.empty())
                        {
                                error_message += '\n';
                        }
                        error_message += thread_error.error_message();
                }
        }

        if (there_is_error)
        {
                error(error_message);
        }
}

ThreadPool::ThreadPool(const unsigned thread_count)
        : thread_count_(thread_count),
          threads_(thread_count > 1 ? thread_count : 0)
{
        for (std::size_t i = 0; i < threads_.size(); ++i)
        {
                threads_[i] = std::thread(
                        [this, i]
                        {
                                thread(i);
                        });
        }
}

unsigned ThreadPool::thread_count() const
{
        return thread_count_;
}

void ThreadPool::run(std::function<void(unsigned, unsigned)>&& function)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        if (threads_.empty())
        {
                constexpr unsigned THREAD_ID = 0;
                constexpr unsigned THREAD_COUNT = 1;

                function(THREAD_ID, THREAD_COUNT);
                return;
        }

        function_ = std::move(function);

        clear_errors();
        start_and_wait();
        find_errors();
}

ThreadPool::~ThreadPool()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        try
        {
                exit_ = true;
                barrier_.arrive_and_wait();

                for (std::thread& t : threads_)
                {
                        t.join();
                }
        }
        catch (...)
        {
                error_fatal("Error in thread pool destructor");
        }
}
}
