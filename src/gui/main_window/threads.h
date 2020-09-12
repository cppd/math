/*
Copyright (C) 2017-2020 Topological Manifold

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

#include <src/progress/progress_list.h>

#include <QProgressBar>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace gui
{
struct WorkerThreads
{
        enum class Action
        {
                Work,
                SelfTest
        };

        struct Progress
        {
                Action action;
                bool permanent;
                const ProgressRatioList* progress_list;
                std::list<QProgressBar>* progress_bars;
        };

        using Function = std::function<void(ProgressRatioList*)>;

        virtual ~WorkerThreads() = default;

        virtual void terminate_with_message(Action action) = 0;
        virtual void terminate_all() = 0;
        virtual bool terminate_and_start(
                Action action,
                const std::string& description,
                std::function<Function()>&& function) = 0;
        virtual const std::vector<Progress>& progresses() const = 0;
};

std::unique_ptr<WorkerThreads> create_worker_threads();
}
