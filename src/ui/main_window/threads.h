/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "catch.h"
#include "event_emitter.h"

#include "progress/progress_list.h"

#include <QProgressBar>

enum class ThreadAction
{
        LoadObject,
        ExportObject,
        ReloadBoundCocone,
        SelfTest
};

struct ThreadProgress
{
        bool permanent;
        const ProgressRatioList* progress_list;
        std::list<QProgressBar>* progress_bars;

        ThreadProgress(bool permanent_, const ProgressRatioList* progress_list_, std::list<QProgressBar>* progress_bars_)
                : permanent(permanent_), progress_list(progress_list_), progress_bars(progress_bars_)
        {
        }
};

class Threads
{
        struct ThreadData
        {
                ProgressRatioList progress_list;
                std::list<QProgressBar> progress_bars;
                std::thread thread;
                std::atomic_bool working = false;

                void stop() noexcept
                {
                        try
                        {
                                progress_list.stop_all();
                                if (thread.joinable())
                                {
                                        thread.join();
                                }
                                progress_list.enable();
                        }
                        catch (...)
                        {
                                error_fatal("Thread stop error");
                        }
                }
        };

        std::thread::id m_thread_id = std::this_thread::get_id();

        const WindowEventEmitter& m_event_emitter;

        std::unordered_map<ThreadAction, ThreadData> m_threads;
        std::vector<ThreadProgress> m_progress;

        bool thread_is_free(ThreadAction action) const
        {
                auto t = m_threads.find(action);
                ASSERT(t != m_threads.end());
                return !(t->second.working);
        }

        ThreadData& action_thread(ThreadAction action)
        {
                auto t = m_threads.find(action);
                ASSERT(t != m_threads.end());
                return t->second;
        }

public:
        Threads(const WindowEventEmitter& emitter) : m_event_emitter(emitter)
        {
                m_threads.try_emplace(ThreadAction::LoadObject);
                m_threads.try_emplace(ThreadAction::ExportObject);
                m_threads.try_emplace(ThreadAction::ReloadBoundCocone);
                m_threads.try_emplace(ThreadAction::SelfTest);

                for (auto& p : m_threads)
                {
                        bool permanent = (p.first == ThreadAction::SelfTest);
                        m_progress.emplace_back(permanent, &p.second.progress_list, &p.second.progress_bars);
                }
        }

        void stop_all_threads() noexcept
        {
                for (auto& t : m_threads)
                {
                        t.second.stop();
                }
        }

        bool action_allowed(ThreadAction action) const
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                switch (action)
                {
                case ThreadAction::LoadObject:
                        return true;
                case ThreadAction::ExportObject:
                        return thread_is_free(ThreadAction::ExportObject);
                case ThreadAction::ReloadBoundCocone:
                        return thread_is_free(ThreadAction::LoadObject);
                case ThreadAction::SelfTest:
                        return true;
                }

                error_fatal("Unknown thread action");
        }

        template <typename F>
        void start_thread(const ThreadAction& thread_action, F&& function)
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (!action_allowed(thread_action))
                {
                        error_fatal("Thread action not allowed");
                }

                switch (thread_action)
                {
                case ThreadAction::LoadObject:
                        action_thread(ThreadAction::ReloadBoundCocone).stop();
                        break;
                case ThreadAction::ExportObject:
                        break;
                case ThreadAction::ReloadBoundCocone:
                        break;
                case ThreadAction::SelfTest:
                        break;
                }

                ThreadData& thread_pack = action_thread(thread_action);
                thread_pack.stop();
                thread_pack.working = true;
                thread_pack.thread = std::thread([ this, &thread_pack, func = std::forward<F>(function) ]() noexcept {

                        catch_all_exceptions(m_event_emitter, [&](std::string* message) {

                                static_assert(!noexcept(func(&thread_pack.progress_list, message)));

                                func(&thread_pack.progress_list, message);
                        });

                        thread_pack.working = false;
                });
        }

        const std::vector<ThreadProgress>& thread_progress()
        {
                return m_progress;
        }
};
