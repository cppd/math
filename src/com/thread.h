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
                std::thread m_thread;
                bool m_has_error = false;
                std::string m_error_message;

        public:
                explicit ThreadData(std::thread&& t) : m_thread(std::move(t))
                {
                }

                void join()
                {
                        m_thread.join();
                }

                void set_error(const char* s)
                {
                        m_has_error = true;
                        m_error_message = s;
                }

                bool has_error() const
                {
                        return m_has_error;
                }

                const std::string& error_message() const
                {
                        return m_error_message;
                }
        };

        const std::thread::id m_thread_id = std::this_thread::get_id();

        std::vector<ThreadData> m_threads;

        void join(bool* there_is_error, std::string* error_message) noexcept
        {
                try
                {
                        try
                        {
                                *there_is_error = false;

                                for (ThreadData& thread_data : m_threads)
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

                                m_threads.clear();
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
        explicit ThreadsWithCatch(unsigned thread_count)
        {
                m_threads.reserve(thread_count);
        }

        ~ThreadsWithCatch()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                if (!m_threads.empty())
                {
                        error_fatal("Thread vector is not empty");
                }
        }

        template <typename F>
        void add(F&& function) noexcept
        {
                try
                {
                        ASSERT(m_thread_id == std::this_thread::get_id());
                        try
                        {
                                auto lambda =
                                        [&, thread_num = m_threads.size(), f = std::forward<F>(function)]() noexcept
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
                                                        m_threads[thread_num].set_error(e.what());
                                                }
                                                catch (...)
                                                {
                                                        m_threads[thread_num].set_error("Unknown error in thread");
                                                }
                                        }
                                        catch (...)
                                        {
                                                error_fatal("Unknown error while calling a thread function");
                                        }
                                };

                                m_threads.emplace_back(std::thread(lambda));
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
                ASSERT(m_thread_id == std::this_thread::get_id());

                bool there_is_error;
                std::string error_message;

                join(&there_is_error, &error_message);

                if (there_is_error)
                {
                        error(error_message);
                }
        }
};

inline void run_in_threads(const std::function<void(std::atomic_size_t&)>& function, std::size_t count)
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
