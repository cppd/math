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

#include <src/progress/progress_list.h>

#include <QProgressBar>
#include <QStatusBar>

#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <string>

namespace ns::gui::com
{
class WorkerThreads
{
public:
        struct Progress final
        {
                unsigned id;
                const progress::RatioList* progress_list;
                std::list<QProgressBar>* progress_bars;
        };

        using Function = std::function<void(progress::RatioList*)>;

        virtual ~WorkerThreads() = default;

        [[nodiscard]] virtual unsigned count() const = 0;

        virtual void terminate_with_message(unsigned id) = 0;

        virtual void terminate_all() = 0;

        virtual bool terminate_and_start(
                unsigned id,
                const std::string& description,
                const std::function<Function()>& function) = 0;

        virtual void set_progresses() = 0;
};

std::unique_ptr<WorkerThreads> create_worker_threads(
        unsigned thread_count,
        const std::optional<unsigned>& permanent_thread_id,
        QStatusBar* status_bar);
}
