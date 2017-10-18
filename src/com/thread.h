/*
Copyright (C) 2017 Topological Manifold

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

inline unsigned get_hardware_concurrency()
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
                catch (...)
                {
                        error_fatal("Error thread barrier wait");
                }
        }
};

class TerminateRequestException final : public std::exception
{
        static constexpr const char m_msg[] = "thread terminate requested";

public:
        TerminateRequestException()
        {
        }
        const char* what() const noexcept override
        {
                return m_msg;
        }
};

template <typename F, typename C, typename... Args>
void launch_class_thread(std::thread* t, std::string* thread_msg, const F& func, const C& cls, const Args&... args)
{
        ASSERT(thread_msg);

        *t = std::thread([=]() noexcept {
                try
                {
                        thread_msg->clear();
                        try
                        {
                                (cls->*func)(args...);
                        }
                        catch (TerminateRequestException&)
                        {
                        }
                        catch (std::exception& e)
                        {
                                *thread_msg = e.what();
                        }
                        catch (...)
                        {
                                *thread_msg = "Unknown error in thread";
                        }
                }
                catch (...)
                {
                        error_fatal("Exception in thread message string.");
                }
        });
}

template <typename F>
void launch_thread(std::thread* t, std::string* thread_msg, const F& f)
{
        ASSERT(thread_msg);

        *t = std::thread([=]() noexcept {
                try
                {
                        thread_msg->clear();
                        try
                        {
                                f();
                        }
                        catch (TerminateRequestException&)
                        {
                        }
                        catch (std::exception& e)
                        {
                                *thread_msg = e.what();
                        }
                        catch (...)
                        {
                                *thread_msg = "Unknown error in thread";
                        }
                }
                catch (...)
                {
                        error_fatal("Exception in thread message string.");
                }
        });
}

inline void join_threads(std::vector<std::thread>* threads, const std::vector<std::string>* msg)
{
        ASSERT(threads);
        ASSERT(msg);
        ASSERT(threads->size() == msg->size());

        for (std::thread& t : *threads)
        {
                t.join();
        }

        std::string e = get_error_list(*msg);
        if (e.size() != 0)
        {
                error(e);
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
};
