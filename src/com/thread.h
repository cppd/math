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

#include "error.h"
#include "exception.h"

#include <algorithm>
#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace ns
{
// positive signed integer
inline int hardware_concurrency()
{
        return std::max(1u, std::thread::hardware_concurrency());
}

class ThreadsWithCatch
{
        class ThreadData
        {
                std::thread thread_;
                bool has_error_ = false;
                std::string error_message_;

        public:
                explicit ThreadData(std::thread&& t) : thread_(std::move(t))
                {
                }

                void join()
                {
                        thread_.join();
                }

                void set_error(const char* const s)
                {
                        has_error_ = true;
                        error_message_ = s;
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

        const std::thread::id thread_id_ = std::this_thread::get_id();

        std::vector<ThreadData> threads_;

        void join(bool* const there_is_error, std::string* const error_message) noexcept
        {
                try
                {
                        try
                        {
                                *there_is_error = false;

                                for (ThreadData& thread_data : threads_)
                                {
                                        thread_data.join();

                                        if (thread_data.has_error())
                                        {
                                                *there_is_error = true;
                                                if (!error_message->empty())
                                                {
                                                        *error_message += '\n';
                                                }
                                                *error_message += thread_data.error_message();
                                        }
                                }

                                threads_.clear();
                        }
                        catch (const std::exception& e)
                        {
                                error_fatal(std::string("Error while joining threads-with-catch: ") + e.what());
                        }
                }
                catch (...)
                {
                        error_fatal("Unknown error while joining threads-with-catch");
                }
        }

public:
        explicit ThreadsWithCatch(const unsigned thread_count)
        {
                threads_.reserve(thread_count);
        }

        ~ThreadsWithCatch()
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                if (!threads_.empty())
                {
                        error_fatal("Thread vector is not empty");
                }
        }

        template <typename F>
        void add(F&& function) noexcept
        {
                try
                {
                        ASSERT(thread_id_ == std::this_thread::get_id());
                        try
                        {
                                auto lambda =
                                        [&, thread_num = threads_.size(), f = std::forward<F>(function)]() noexcept
                                {
                                        try
                                        {
                                                try
                                                {
                                                        f();
                                                }
                                                catch (const TerminateQuietlyException&)
                                                {
                                                }
                                                catch (const std::exception& e)
                                                {
                                                        threads_[thread_num].set_error(e.what());
                                                }
                                                catch (...)
                                                {
                                                        threads_[thread_num].set_error("Unknown error in thread");
                                                }
                                        }
                                        catch (...)
                                        {
                                                error_fatal("Unknown error while calling a thread function");
                                        }
                                };

                                threads_.emplace_back(std::thread(lambda));
                        }
                        catch (const std::exception& e)
                        {
                                error_fatal(std::string("Error while adding a thread-with-catch: ") + e.what());
                        }
                }
                catch (...)
                {
                        error_fatal("Unknown error while adding a thread-with-catch");
                }
        }

        void join()
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                bool there_is_error;
                std::string error_message;

                join(&there_is_error, &error_message);

                if (there_is_error)
                {
                        error(error_message);
                }
        }
};

inline void run_in_threads(const std::function<void(std::atomic_size_t&)>& function, const std::size_t count)
{
        std::size_t concurrency = hardware_concurrency();
        unsigned thread_count = std::min(count, concurrency);
        std::atomic_size_t task = 0;
        if (thread_count > 1)
        {
                ThreadsWithCatch threads(thread_count);
                for (unsigned i = 0; i < thread_count; ++i)
                {
                        threads.add(
                                [&]()
                                {
                                        function(task);
                                });
                }
                threads.join();
        }
        else if (thread_count == 1)
        {
                function(task);
        }
}
}
