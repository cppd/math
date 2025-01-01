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

#include "error.h"
#include "exception.h"

#include <atomic>
#include <barrier>
#include <exception>
#include <functional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace ns
{
class ThreadPool final
{
        class ThreadError final
        {
                bool has_error_;
                std::string error_message_;

        public:
                void set(const char* const s)
                {
                        has_error_ = true;
                        error_message_ = s;
                }

                void clear()
                {
                        has_error_ = false;
                        error_message_.clear();
                }

                [[nodiscard]] bool has_error() const
                {
                        return has_error_;
                }

                [[nodiscard]] const std::string& error_message() const
                {
                        return error_message_;
                }
        };

        const std::thread::id thread_id_ = std::this_thread::get_id();

        const unsigned thread_count_;

        std::barrier<> barrier_{thread_count_ + 1};
        std::atomic_bool exit_{false};

        std::vector<std::thread> threads_{thread_count_};
        std::vector<ThreadError> thread_errors_{thread_count_};

        std::function<void(unsigned, unsigned)> function_;

        void process(const unsigned thread_num) noexcept
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
                                error_fatal(
                                        std::string("Exception in thread pool while working with exception: ")
                                        + e.what());
                        }
                }
                catch (...)
                {
                        error_fatal("Exception in thread pool while working with exception");
                }
        }

        void thread(const unsigned thread_num) noexcept
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

        void start_and_wait() noexcept
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

        void clear_errors()
        {
                for (ThreadError& thread_error : thread_errors_)
                {
                        thread_error.clear();
                }
        }

        void find_errors() const
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

public:
        explicit ThreadPool(const unsigned thread_count)
                : thread_count_(thread_count)
        {
                for (unsigned i = 0; i < thread_count_; ++i)
                {
                        threads_[i] = std::thread(
                                [this, i]
                                {
                                        thread(i);
                                });
                }
        }

        [[nodiscard]] unsigned thread_count() const
        {
                return thread_count_;
        }

        void run(std::function<void(unsigned, unsigned)>&& function)
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                function_ = std::move(function);

                clear_errors();
                start_and_wait();
                find_errors();
        }

        ~ThreadPool()
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
};
}
