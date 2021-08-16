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

#include <condition_variable>
#include <mutex>

namespace ns
{
class Barrier
{
        std::mutex mutex_;
        std::condition_variable cv_;
        int count_;
        const int thread_count_;
        long long generation_ = 0;

public:
        explicit Barrier(int thread_count) : count_(thread_count), thread_count_(thread_count)
        {
        }

        void wait() noexcept
        {
                try
                {
                        try
                        {
                                if (thread_count_ == 1)
                                {
                                        return;
                                }

                                std::unique_lock<std::mutex> lock(mutex_);
                                long long g = generation_;
                                --count_;
                                if (count_ == 0)
                                {
                                        ++generation_;
                                        count_ = thread_count_;
                                        cv_.notify_all();
                                }
                                else
                                {
                                        cv_.wait(
                                                lock,
                                                [this, g]
                                                {
                                                        return g != generation_;
                                                });
                                }
                        }
                        catch (const std::exception& e)
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
}
