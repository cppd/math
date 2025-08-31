/*
Copyright (C) 2017-2025 Topological Manifold

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
#include <optional>
#include <stack>
#include <vector>

namespace ns
{
template <typename Task>
class ThreadTasks final
{
        template <typename T>
        friend class ThreadTaskManager;

        // If there are no tasks and all thread do nothing, then there will be no more tasks.
        // If there are no tasks and a thread do something, then new tasks can be created.
        // Instead of counting tasks for each thread, the sum of tasks across all threads is used.
        // A thread requires a new task without having a task - the sum is the same.
        // A thread requires a new task having a task - the sum decreases by 1.
        // A thread gets a new task - the sum increases by 1.
        int task_count_ = 0;

        std::stack<Task, std::vector<Task>> tasks_;
        std::mutex lock_;
        bool stop_ = false;

        void release(const bool has_task)
        {
                if (!has_task)
                {
                        return;
                }
                const std::lock_guard lg(lock_);
                --task_count_;
        }

        std::optional<Task> get(bool has_task)
        {
                while (true)
                {
                        const std::lock_guard lg(lock_);
                        if (stop_)
                        {
                                return {};
                        }
                        if (has_task)
                        {
                                --task_count_;
                                has_task = false;
                        }
                        if (!tasks_.empty())
                        {
                                Task task = std::move(tasks_.top());
                                tasks_.pop();
                                ++task_count_;
                                return task;
                        }
                        if (task_count_ == 0)
                        {
                                return {};
                        }
                }
        }

public:
        template <typename... Ts>
        void emplace(Ts&&... vs)
        {
                const std::lock_guard lg(lock_);
                tasks_.emplace(std::forward<Ts>(vs)...);
        }

        void stop()
        {
                const std::lock_guard lg(lock_);
                stop_ = true;
        }
};

template <typename Task>
class ThreadTaskManager final
{
        ThreadTasks<Task>* tasks_;
        bool has_task_ = false;

public:
        explicit ThreadTaskManager(ThreadTasks<Task>* const tasks)
                : tasks_(tasks)
        {
        }

        ~ThreadTaskManager()
        {
                tasks_->release(has_task_);
        }

        ThreadTaskManager(const ThreadTaskManager&) = delete;
        ThreadTaskManager& operator=(const ThreadTaskManager&) = delete;
        ThreadTaskManager(ThreadTaskManager&&) = delete;
        ThreadTaskManager& operator=(ThreadTaskManager&&) = delete;

        std::optional<Task> get()
        {
                std::optional<Task> task = tasks_->get(has_task_);
                has_task_ = task.has_value();
                return task;
        }

        template <typename... Ts>
        void emplace(Ts&&... vs)
        {
                tasks_->emplace(std::forward<Ts>(vs)...);
        }
};
}
