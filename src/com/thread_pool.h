/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "com/log.h"
#include "com/print.h"
#include "com/thread.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

template <typename... T>
class ThreadPool
{
        const unsigned THREAD_COUNT;

        std::mutex m_mutex_run, m_mutex_finish;
        std::condition_variable m_cv_run, m_cv_finish;

        std::vector<std::thread> m_threads;
        std::vector<std::string> m_msg;
        std::vector<std::string> m_thread_errors;

        bool m_enable;
        int m_count;
        long long m_generation;
        bool m_exit;

        std::function<void(unsigned, unsigned)> m_bound_function;

        bool start_in_thread() noexcept
        {
                try
                {
                        std::unique_lock<std::mutex> lock(m_mutex_run);
                        m_cv_run.wait(lock, [this] { return m_enable || m_exit; });
                        return !m_exit;
                }
                catch (std::exception& e)
                {
                        error_fatal(std::string("exception in thread pool start code: ") + e.what());
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
                                m_cv_finish.wait(lock, [this, g] { return g != m_generation; });
                        }
                }
                catch (std::exception& e)
                {
                        error_fatal(std::string("exception in thread pool finish code: ") + e.what());
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
                                m_bound_function(thread_num, THREAD_COUNT);
                        }
                        catch (std::exception& e)
                        {
                                m_thread_errors[thread_num] = e.what();
                        }
                        catch (...)
                        {
                                m_thread_errors[thread_num] = "Unknown error in thread of thread pool";
                        }
                }
                catch (std::exception& e)
                {
                        error_fatal(std::string("exception in thread pool while working with exception: ") + e.what());
                }
                catch (...)
                {
                        error_fatal("exception in thread pool while working with exception");
                }
        }

        void thread(unsigned thread_num) noexcept
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

        void start_and_wait() noexcept
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
                                m_cv_finish.wait(lock, [this, g] { return g != m_generation; });
                        }
                }
                catch (std::exception& e)
                {
                        error_fatal(std::string("Error start and wait threads: ") + e.what());
                }
                catch (...)
                {
                        error_fatal("Error start and wait threads");
                }
        }

public:
        ThreadPool(unsigned thread_count)
                : THREAD_COUNT(thread_count), m_threads(THREAD_COUNT), m_msg(THREAD_COUNT), m_thread_errors(THREAD_COUNT)
        {
                m_enable = false;
                m_exit = false;
                m_generation = 0;
                m_count = THREAD_COUNT;

                for (unsigned i = 0; i < m_threads.size(); ++i)
                {
                        launch_class_thread(&m_threads[i], &m_msg[i], &ThreadPool::thread, this, i);
                }
        }
        unsigned get_thread_count() const
        {
                return THREAD_COUNT;
        }
        void run(const std::function<void(unsigned, unsigned, T...)>& function, T... args)
        {
                m_bound_function = std::bind(function, std::placeholders::_1, std::placeholders::_2, args...);

                // Очистка сообщений об ошибках
                for (std::string& s : m_thread_errors)
                {
                        s.clear();
                }

                start_and_wait();

                // Список возможных ошибок из потоков
                std::string e = thread_errors_to_string_lines(m_thread_errors);
                if (e.size() != 0)
                {
                        error(e);
                }
        }
        ~ThreadPool()
        {
                {
                        std::unique_lock<std::mutex> lock(m_mutex_run);
                        m_exit = true;
                }

                m_cv_run.notify_all();

                for (std::thread& t : m_threads)
                {
                        t.join();
                }

                std::string e = thread_errors_to_string_lines(m_msg);
                if (e.size() != 0)
                {
                        error_fatal(std::string("Thread pool thread(s) exited with error: ") + e);
                }
        }
};
