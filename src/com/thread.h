/*
Copyright (C) 2017-2019 Topological Manifold

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

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <vector>

// Нужно целое число со знаком, больше или равное 1
inline int hardware_concurrency()
{
        return std::max(1u, std::thread::hardware_concurrency());
}

class SpinLock
{
        std::atomic_flag spin_lock;

public:
        SpinLock()
        {
                spin_lock.clear();
        }
        void lock()
        {
                while (spin_lock.test_and_set(std::memory_order_acquire))
                {
                }
        }
        void unlock()
        {
                spin_lock.clear(std::memory_order_release);
        }
};

template <typename T>
class ThreadQueue
{
        SpinLock m_lock;
        std::queue<T> m_queue;

public:
        std::optional<T> pop()
        {
                std::lock_guard lg(m_lock);
                if (m_queue.empty())
                {
                        return std::optional<T>();
                }
                else
                {
                        std::optional value(std::move(m_queue.front()));
                        m_queue.pop();
                        return value;
                }
        }
        // template <typename A>
        // void push(A&& e)
        //{
        //        std::lock_guard lg(m_lock);
        //        m_queue.push(std::forward<A>(e));
        //}
        template <typename... Args>
        void emplace(Args&&... e)
        {
                std::lock_guard lg(m_lock);
                m_queue.emplace(std::forward<Args>(e)...);
        }
        void clear()
        {
                std::lock_guard lg(m_lock);
                m_queue = std::queue<T>();
        }
};

class ThreadBarrier
{
        std::mutex m_mutex;
        std::condition_variable m_cv;
        int m_count;
        const int m_thread_count;
        long long m_generation = 0;

public:
        ThreadBarrier(int thread_count) : m_count(thread_count), m_thread_count(thread_count)
        {
        }
        void wait() noexcept
        {
                try
                {
                        try
                        {
                                if (m_thread_count == 1)
                                {
                                        return;
                                }

                                std::unique_lock<std::mutex> lock(m_mutex);
                                long long g = m_generation;
                                --m_count;
                                if (m_count == 0)
                                {
                                        ++m_generation;
                                        m_count = m_thread_count;
                                        m_cv.notify_all();
                                }
                                else
                                {
                                        m_cv.wait(lock, [this, g] { return g != m_generation; });
                                }
                        }
                        catch (std::exception& e)
                        {
                                error_fatal(std::string("Error thread barrier wait: ") + e.what());
                        }
                }
                catch (...)
                {
                        error_fatal("Error thread barrier wait");
                }
        }
};

class TerminateRequestException final : public std::exception
{
        static constexpr const char m_msg[] = "Thread termination requested";

public:
        TerminateRequestException()
        {
        }
        const char* what() const noexcept override
        {
                return m_msg;
        }
};

class TerminateWithMessageRequestException final : public std::exception
{
        static constexpr const char m_msg[] = "Terminated by user";

public:
        TerminateWithMessageRequestException()
        {
        }
        const char* what() const noexcept override
        {
                return m_msg;
        }
};

[[noreturn]] inline void throw_terminate_quietly_exception()
{
        throw TerminateRequestException();
}

[[noreturn]] inline void throw_terminate_with_message_exception()
{
        throw TerminateWithMessageRequestException();
}

class ThreadsWithCatch
{
        class ThreadData
        {
                std::thread m_thread;
                bool m_has_error = false;
                std::string m_error_message;

        public:
                ThreadData(std::thread&& t) : m_thread(std::move(t))
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

        void join(bool& there_is_error, std::string& error_message) noexcept
        {
                try
                {
                        try
                        {
                                there_is_error = false;

                                for (ThreadData& thread_data : m_threads)
                                {
                                        thread_data.join();

                                        if (thread_data.has_error())
                                        {
                                                there_is_error = true;
                                                if (error_message.size() > 0)
                                                {
                                                        error_message += '\n';
                                                }
                                                error_message += thread_data.error_message();
                                        }
                                }

                                m_threads.clear();
                        }
                        catch (std::exception& e)
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
        ThreadsWithCatch(unsigned thread_count)
        {
                m_threads.reserve(thread_count);
        }

        ~ThreadsWithCatch()
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                if (m_threads.size() != 0)
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
                                auto lambda = [&, thread_num = m_threads.size(), f = std::forward<F>(function)]() noexcept {
                                        try
                                        {
                                                try
                                                {
                                                        f();
                                                }
                                                catch (TerminateRequestException&)
                                                {
                                                }
                                                catch (std::exception& e)
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
                        catch (std::exception& e)
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

                join(there_is_error, error_message);

                if (there_is_error)
                {
                        error(error_message);
                }
        }
};

inline void run_in_threads(const std::function<void(std::atomic_size_t&)>& function, size_t count)
{
        size_t concurrency = hardware_concurrency();
        unsigned thread_count = std::min(count, concurrency);
        std::atomic_size_t task = 0;
        if (thread_count > 1)
        {
                ThreadsWithCatch threads(thread_count);
                for (unsigned i = 0; i < thread_count; ++i)
                {
                        threads.add([&]() { function(task); });
                }
                threads.join();
        }
        else if (thread_count == 1)
        {
                function(task);
        }
}

template <typename T>
class AtomicCounter
{
        static_assert(std::atomic<T>::is_always_lock_free);

        std::atomic<T> m_counter;

public:
        static constexpr bool is_always_lock_free = std::atomic<T>::is_always_lock_free;

        AtomicCounter() : m_counter(0)
        {
        }
        AtomicCounter(T v) : m_counter(v)
        {
        }
        void operator=(T v)
        {
                m_counter.store(v, std::memory_order_relaxed);
        }
        operator T() const
        {
                return m_counter.load(std::memory_order_relaxed);
        }
        void operator++()
        {
                m_counter.fetch_add(1, std::memory_order_relaxed);
        }
        void operator+=(T v)
        {
                m_counter.fetch_add(v, std::memory_order_relaxed);
        }
};
