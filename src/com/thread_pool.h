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
        const unsigned THREAD_COUNT;

        std::mutex m_mutex_run, m_mutex_finish;
        std::condition_variable m_cv_run, m_cv_finish;

        ThreadsWithCatch m_threads;

        class ThreadError
        {
                bool m_has_error;
                std::string m_error_message;

        public:
                void set(const char* s)
                {
                        m_has_error = true;
                        m_error_message = s;
                }
                void clear()
                {
                        m_has_error = false;
                        m_error_message.clear();
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

        std::vector<ThreadError> m_thread_errors;

        bool m_enable;
        int m_count;
        long long m_generation;
        bool m_exit;

        std::function<void(unsigned, unsigned)> m_bound_function;

        bool start_in_thread() noexcept
        {
                try
                {
                        try
                        {
                                std::unique_lock<std::mutex> lock(m_mutex_run);
                                m_cv_run.wait(
                                        lock,
                                        [this]
                                        {
                                                return m_enable || m_exit;
                                        });
                                return !m_exit;
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
                                std::unique_lock<std::mutex> lock(m_mutex_finish);

                                long long g = m_generation;
                                --m_count;
                                if (m_count == 0)
                                {
                                        m_enable = false;
                                        m_count = THREAD_COUNT;
                                        ++m_generation;
                                        // Извещение идёт также и вызывающему потоку, что тут закончилась работа
                                        m_cv_finish.notify_all();
                                }
                                else
                                {
                                        m_cv_finish.wait(
                                                lock,
                                                [this, g]
                                                {
                                                        return g != m_generation;
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
                                        m_bound_function(thread_num, THREAD_COUNT);
                                }
                                catch (TerminateRequestException&)
                                {
                                }
                                catch (const std::exception& e)
                                {
                                        m_thread_errors[thread_num].set(e.what());
                                }
                                catch (...)
                                {
                                        m_thread_errors[thread_num].set("Unknown error in thread of thread pool");
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
                                long long g = m_generation;

                                // Запуск потоков
                                {
                                        std::unique_lock<std::mutex> lock(m_mutex_run);
                                        m_enable = true;
                                }

                                m_cv_run.notify_all();

                                // Ожидание завершения потоков
                                {
                                        std::unique_lock<std::mutex> lock(m_mutex_finish);
                                        m_cv_finish.wait(
                                                lock,
                                                [this, g]
                                                {
                                                        return g != m_generation;
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
                for (ThreadError& thread_error : m_thread_errors)
                {
                        thread_error.clear();
                }
        }

        void find_errors() const
        {
                bool there_is_error = false;
                std::string error_message;

                for (const ThreadError& thread_error : m_thread_errors)
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
                : THREAD_COUNT(thread_count), m_threads(THREAD_COUNT), m_thread_errors(THREAD_COUNT)
        {
                m_enable = false;
                m_exit = false;
                m_generation = 0;
                m_count = THREAD_COUNT;

                for (unsigned i = 0; i < THREAD_COUNT; ++i)
                {
                        m_threads.add(
                                [&, i]()
                                {
                                        thread(i);
                                });
                }
        }

        unsigned thread_count() const
        {
                return THREAD_COUNT;
        }

        void run(std::function<void(unsigned, unsigned)>&& function)
        {
                m_bound_function = std::move(function);

                clear_errors();

                start_and_wait();

                find_errors();
        }

        ~ThreadPool()
        {
                {
                        std::unique_lock<std::mutex> lock(m_mutex_run);
                        m_exit = true;
                }

                m_cv_run.notify_all();

                try
                {
                        m_threads.join();
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
