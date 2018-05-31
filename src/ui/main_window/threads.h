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

#include "progress/progress_list.h"

#include <QProgressBar>
#include <exception>
#include <functional>

enum class ThreadAction
{
        LoadObject,
        ExportObject,
        ReloadBoundCocone,
        SelfTest
};

struct ThreadProgress
{
        ThreadAction thread_action;
        bool permanent;
        const ProgressRatioList* progress_list;
        std::list<QProgressBar>* progress_bars;

        ThreadProgress(ThreadAction thread_action_, bool permanent_, const ProgressRatioList* progress_list_,
                       std::list<QProgressBar>* progress_bars_)
                : thread_action(thread_action_),
                  permanent(permanent_),
                  progress_list(progress_list_),
                  progress_bars(progress_bars_)
        {
        }
};

class Threads
{
        class ThreadData
        {
                enum class StopType
                {
                        WithMessage,
                        Silent
                };

                void stop(StopType stop_type) noexcept
                {
                        try
                        {
                                switch (stop_type)
                                {
                                case StopType::WithMessage:
                                        progress_list.stop_all_with_message();
                                        break;
                                case StopType::Silent:
                                        progress_list.stop_all();
                                        break;
                                }

                                if (thread.joinable())
                                {
                                        thread.join();
                                }

                                progress_list.enable();
                        }
                        catch (...)
                        {
                                switch (stop_type)
                                {
                                case StopType::WithMessage:
                                        error_fatal("Thread stop with message error");
                                case StopType::Silent:
                                        error_fatal("Thread stop error");
                                };
                        }
                }

        public:
                ProgressRatioList progress_list;
                std::list<QProgressBar> progress_bars;
                std::thread thread;
                std::atomic_bool working = false;

                void stop_with_message() noexcept
                {
                        stop(StopType::WithMessage);
                }

                void stop_silent() noexcept
                {
                        stop(StopType::Silent);
                }
        };

        const std::thread::id m_thread_id = std::this_thread::get_id();

        std::function<void(const std::exception_ptr& ptr, const std::string& msg)> m_exception_handler;

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
        Threads(std::function<void(const std::exception_ptr& ptr, const std::string& msg)> exception_handler)
                : m_exception_handler(exception_handler)
        {
                m_threads.try_emplace(ThreadAction::LoadObject);
                m_threads.try_emplace(ThreadAction::ExportObject);
                m_threads.try_emplace(ThreadAction::ReloadBoundCocone);
                m_threads.try_emplace(ThreadAction::SelfTest);

                for (auto& p : m_threads)
                {
                        bool permanent = (p.first == ThreadAction::SelfTest);
                        m_progress.emplace_back(p.first, permanent, &p.second.progress_list, &p.second.progress_bars);
                }
        }

        void stop_thread_with_message(ThreadAction action) noexcept
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                action_thread(action).stop_with_message();
        }

        void stop_all_threads() noexcept
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                for (auto& t : m_threads)
                {
                        t.second.stop_silent();
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
                        action_thread(ThreadAction::ReloadBoundCocone).stop_silent();
                        break;
                case ThreadAction::ExportObject:
                        break;
                case ThreadAction::ReloadBoundCocone:
                        break;
                case ThreadAction::SelfTest:
                        break;
                }

                ThreadData& thread_pack = action_thread(thread_action);
                thread_pack.stop_silent();
                thread_pack.working = true;
                thread_pack.thread = std::thread([ this, &thread_pack, func = std::forward<F>(function) ]() noexcept {
                        std::string message;
                        try
                        {
                                static_assert(!noexcept(func(&thread_pack.progress_list, &message)));

                                func(&thread_pack.progress_list, &message);
                        }
                        catch (...)
                        {
                                m_exception_handler(std::current_exception(), message);
                        }
                        thread_pack.working = false;
                });
        }

        const std::vector<ThreadProgress>& thread_progress()
        {
                return m_progress;
        }
};
