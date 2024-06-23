/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <mutex>
#include <queue>
#include <utility>
#include <vector>

namespace ns::view
{
template <typename T>
class ThreadQueue final
{
        std::mutex lock_;
        std::queue<T> queue_;

public:
        template <typename Type>
        void push(Type&& v)
        {
                const std::lock_guard lg(lock_);
                queue_.push(std::forward<Type>(v));
        }

        [[nodiscard]] std::vector<T> pop()
        {
                static_assert(std::is_nothrow_move_assignable_v<T>);
                static_assert(std::is_nothrow_destructible_v<T>);

                std::vector<T> res;

                {
                        const std::lock_guard lg(lock_);
                        res.reserve(queue_.size());
                        while (!queue_.empty())
                        {
                                res.push_back(std::move(queue_.front()));
                                queue_.pop();
                        }
                }

                return res;
        }
};
}
