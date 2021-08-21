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

#include "exception.h"
#include "log.h"
#include "print.h"
#include "thread.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace ns
{
class ThreadPool
{
        const unsigned thread_count_;

        std::mutex mutex_run_, mutex_finish_;
        std::condition_variable cv_run_, cv_finish_;

        ThreadsWithCatch threads_;

        class ThreadError
        {
                bool has_error_;
                std::string error_message_;

        public:
                void set(const char* s)
                {
                        has_error_ = true;
                        error_message_ = s;
                }
                void clear()
                {
                        has_error_ = false;
                        error_message_.clear();
                }
                bool has_error() const
                {
                        return has_error_;
                }
                const std::string& error_message() const
                {
                        return error_message_;
                }
        };

        std::vector<ThreadError> thread_errors_;

        bool enable_;
        int count_;
        long long generation_;
        bool exit_;

        std::function<void(unsigned, unsigned)> bound_function_;

        bool start_in_thread() noexcept
        {
                try
                {
                        try
                        {
                                std::unique_lock<std::mutex> lock(mutex_run_);
                                cv_run_.wait(
                                        lock,
                                        [this]
                                        {
                                                return enable_ || exit_;
                                        });
                                return !exit_;
                        }
                        catch (const std::exception& e)
                        {
                                error_fatal(std::string("exception in thread pool start code: ") + e.what());
                        }
                }
                catch (...)
                {
                        error_fatal("exception in thread pool start code");
                }
        }

        void finish_in_thread() noexcept
        {
                try
                {
                        try
                        {
                                std::unique_lock<std::mutex> lock(mutex_finish_);

                                long long g = generation_;
                                --count_;
                                if (count_ == 0)
                                {
                                        enable_ = false;
                                        count_ = thread_count_;
                                        ++generation_;
                                        cv_finish_.notify_all();
                                }
                                else
                                {
                                        cv_finish_.wait(
                                                lock,
                                                [this, g]
                                                {
                                                        return g != generation_;
                                                });
                                }
                        }
                        catch (const std::exception& e)
                        {
                                error_fatal(std::string("exception in thread pool finish code: ") + e.what());
                        }
                }
                catch (...)
                {
                        error_fatal("exception in thread pool finish code");
                }
        }

        void process(unsigned thread_num) noexcept
        {
                try
                {
                        try
                        {
                                try
                                {
                                        bound_function_(thread_num, thread_count_);
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
                                        thread_errors_[thread_num].set("Unknown error in thread of thread pool");
                                }
                        }
                        catch (const std::exception& e)
                        {
                                error_fatal(
                                        std::string("exception in thread pool while working with exception: ")
                                        + e.what());
                        }
                }
                catch (...)
                {
                        error_fatal("exception in thread pool while working with exception");
                }
        }

        void thread(unsigned thread_num) noexcept
        {
                try
                {
                        while (true)
                        {
                                if (!start_in_thread())
                                {
                                        return;
                                }

                                process(thread_num);

                                finish_in_thread();
                        }
                }
                catch (...)
                {
                        error_fatal("exception in thread pool while proccessing thread");
                }
        }

        void start_and_wait() noexcept
        {
                try
                {
                        try
                        {
                                long long g = generation_;

                                {
                                        std::unique_lock<std::mutex> lock(mutex_run_);
                                        enable_ = true;
                                }

                                cv_run_.notify_all();

                                {
                                        std::unique_lock<std::mutex> lock(mutex_finish_);
                                        cv_finish_.wait(
                                                lock,
                                                [this, g]
                                                {
                                                        return g != generation_;
                                                });
                                }
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
        explicit ThreadPool(unsigned thread_count)
                : thread_count_(thread_count), threads_(thread_count_), thread_errors_(thread_count_)
        {
                enable_ = false;
                exit_ = false;
                generation_ = 0;
                count_ = thread_count_;

                for (unsigned i = 0; i < thread_count_; ++i)
                {
                        threads_.add(
                                [&, i]()
                                {
                                        thread(i);
                                });
                }
        }

        unsigned thread_count() const
        {
                return thread_count_;
        }

        void run(std::function<void(unsigned, unsigned)>&& function)
        {
                bound_function_ = std::move(function);

                clear_errors();

                start_and_wait();

                find_errors();
        }

        ~ThreadPool()
        {
                {
                        std::unique_lock<std::mutex> lock(mutex_run_);
                        exit_ = true;
                }

                cv_run_.notify_all();

                try
                {
                        threads_.join();
                }
                catch (const std::exception& e)
                {
                        error_fatal(std::string("Thread pool thread(s) exited with error(s): ") + e.what());
                }
                catch (...)
                {
                        error_fatal("Thread pool thread(s) exited with an unknown error");
                }
        }
};
}
