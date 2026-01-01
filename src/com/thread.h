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

#pragma once

#include "error.h"
#include "exception.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <future>
#include <optional>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace ns
{
// positive signed integer
inline int hardware_concurrency()
{
        return std::max<int>(1, std::thread::hardware_concurrency());
}

class Threads final
{
        struct Thread final
        {
                std::future<void> future;
                std::thread thread;

                explicit Thread(std::packaged_task<void()>&& task)
                        : future(task.get_future()),
                          thread(std::move(task))
                {
                }
        };

        const std::thread::id thread_id_ = std::this_thread::get_id();

        std::vector<Thread> threads_;

        std::optional<std::string> join_threads()
        {
                std::optional<std::string> res;

                const auto add_message = [&](const char* const message)
                {
                        if (!res)
                        {
                                res = message;
                                return;
                        }
                        if (!res->empty())
                        {
                                *res += '\n';
                        }
                        *res += message;
                };

                for (Thread& thread : threads_)
                {
                        thread.thread.join();
                        try
                        {
                                thread.future.get();
                        }
                        catch (const TerminateQuietlyException&)
                        {
                        }
                        catch (const std::exception& e)
                        {
                                add_message(e.what());
                        }
                        catch (...)
                        {
                                add_message("Unknown error in thread");
                        }
                }

                threads_.clear();

                return res;
        }

public:
        explicit Threads(const unsigned thread_count)
        {
                threads_.reserve(thread_count);
        }

        ~Threads()
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                if (!threads_.empty())
                {
                        error_fatal("Threads are not joined");
                }
        }

        template <typename F>
                requires (!std::is_same_v<std::packaged_task<void()>, std::remove_cvref_t<F>>)
        void add(F&& f) noexcept
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                try
                {
                        try
                        {
                                threads_.emplace_back(std::packaged_task<void()>(std::forward<F>(f)));
                        }
                        catch (const std::exception& e)
                        {
                                error_fatal(std::string("Error while adding a thread: ") + e.what());
                        }
                }
                catch (...)
                {
                        error_fatal("Unknown error while adding a thread");
                }
        }

        void join()
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                std::optional<std::string> error_message;

                try
                {
                        try
                        {
                                error_message = join_threads();
                        }
                        catch (const std::exception& e)
                        {
                                error_fatal(std::string("Error while joining threads: ") + e.what());
                        }
                }
                catch (...)
                {
                        error_fatal("Unknown error while joining threads");
                }

                if (error_message)
                {
                        error(*error_message);
                }
        }
};

template <typename F>
void run_in_threads(const F& f, const std::size_t count)
{
        const std::size_t concurrency = hardware_concurrency();
        const unsigned thread_count = std::min(count, concurrency);
        std::atomic_size_t task = 0;
        if (thread_count > 1)
        {
                Threads threads(thread_count);
                for (unsigned i = 0; i < thread_count; ++i)
                {
                        threads.add(
                                [&]
                                {
                                        f(task);
                                });
                }
                threads.join();
        }
        else if (thread_count == 1)
        {
                f(task);
        }
}

inline void join_thread(std::thread* const thread) noexcept
{
        ASSERT(thread);

        try
        {
                try
                {
                        if (thread->joinable())
                        {
                                thread->join();
                        }
                }
                catch (const std::exception& e)
                {
                        error_fatal(std::string("Error joining thread ") + e.what());
                }
                catch (...)
                {
                        error_fatal("Unknown error joining thread");
                }
        }
        catch (...)
        {
                error_fatal("Exception in join thread exception handlers");
        }
}
}
