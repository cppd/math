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

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

inline unsigned get_hardware_concurrency()
{
        unsigned hc = std::thread::hardware_concurrency();
        return hc > 0 ? hc : 1;
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
        bool pop(T& e)
        {
                std::lock_guard<SpinLock> lg(m_lock);
                if (m_queue.empty())
                {
                        return false;
                }
                else
                {
                        e = std::move(m_queue.front());
                        m_queue.pop();
                        return true;
                }
        }
        // template <typename A>
        // void push(A&& e)
        //{
        //        std::lock_guard<SpinLock> lg(m_lock);
        //        m_queue.push(std::forward<A>(e));
        //}
        template <typename... Args>
        void emplace(Args&&... e)
        {
                std::lock_guard<SpinLock> lg(m_lock);
                m_queue.emplace(std::forward<Args>(e)...);
        }
        void clear()
        {
                std::lock_guard<SpinLock> lg(m_lock);
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
        static constexpr const char* m_msg = "thread terminate requested";

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
void thread_function(std::string* thread_msg, F&& func, C&& cls, Args&&... args)
{
        try
        {
                if (thread_msg)
                {
                        thread_msg->clear();
                }

                try
                {
                        (cls->*func)(std::forward<Args>(args)...);
                }
                catch (TerminateRequestException&)
                {
                }
                catch (std::exception& e)
                {
                        if (thread_msg)
                        {
                                *thread_msg = e.what();
                        }
                }
                catch (...)
                {
                        if (thread_msg)
                        {
                                *thread_msg = "Unknown thread error";
                        }
                }
        }
        catch (...)
        {
                error_fatal("Critical program error. Exception in thread message string.");
        }
}

template <typename F, typename C, typename... Args>
void launch_class_thread(std::thread* t, std::string* thread_msg, F func, C cls, const Args&... args)
{
        *t = std::thread(thread_function<F, C, Args...>, thread_msg, func, cls, args...);
}

template <typename F, typename C, typename... Args>
void launch_class_detached_thread(F func, C cls, const Args&... args)
{
        std::thread t(thread_function<F, C, Args...>, nullptr, func, cls, args...);
        t.detach();
}

inline void join_threads(std::vector<std::thread>* threads, const std::vector<std::string>* msg)
{
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
