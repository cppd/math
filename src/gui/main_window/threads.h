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
#include <exception>
#include <functional>
#include <list>
#include <memory>

struct MainThreads
{
        enum class Action
        {
                Load,
                Export,
                ReloadBoundCocone,
                SelfTest
        };

        struct Progress
        {
                Action action;
                bool permanent;
                const ProgressRatioList* progress_list;
                std::list<QProgressBar>* progress_bars;
        };

        virtual ~MainThreads() = default;

        virtual void terminate_thread_with_message(Action action) = 0;
        virtual void terminate_all_threads() = 0;

        virtual bool action_allowed(Action action) const = 0;

        virtual void start_thread(
                Action action,
                const std::function<void(ProgressRatioList*, std::string*)>& function) = 0;

        virtual const std::vector<Progress>& thread_progress() const = 0;
};

std::unique_ptr<MainThreads> create_main_threads(
        const std::function<void(const std::exception_ptr& ptr, const std::string& msg)>& exception_handler);
