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

#include <atomic>
#include <barrier>
#include <cstddef>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace ns
{
class ThreadPool final
{
        class ThreadError final
        {
                bool has_error_;
                std::string error_message_;

        public:
                void set(const char* const s)
                {
                        has_error_ = true;
                        error_message_ = s;
                }

                void clear()
                {
                        has_error_ = false;
                        error_message_.clear();
                }

                [[nodiscard]] bool has_error() const
                {
                        return has_error_;
                }

                [[nodiscard]] const std::string& error_message() const
                {
                        return error_message_;
                }
        };

        const std::thread::id thread_id_ = std::this_thread::get_id();

        const unsigned thread_count_;
        std::atomic_bool exit_{false};
        std::vector<std::thread> threads_;
        std::vector<ThreadError> thread_errors_{threads_.size()};
        std::barrier<> barrier_{static_cast<std::ptrdiff_t>(threads_.size() + 1)};

        std::function<void(unsigned, unsigned)> function_;

        void process(unsigned thread_num) noexcept;

        void thread(unsigned thread_num) noexcept;

        void start_and_wait() noexcept;

        void clear_errors();

        void find_errors() const;

public:
        explicit ThreadPool(unsigned thread_count);

        [[nodiscard]] unsigned thread_count() const;

        void run(std::function<void(unsigned, unsigned)>&& function);

        ~ThreadPool();
};
}
