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

#include "threads.h"

#include <atomic>
#include <thread>
#include <unordered_map>

namespace
{
class ThreadData
{
        ProgressRatioList m_progress_list;
        std::list<QProgressBar> m_progress_bars;
        std::thread m_thread;
        std::atomic_bool m_working = false;
        const std::function<void(const std::exception_ptr& ptr, const std::string& msg)>& m_exception_handler;

        enum class TerminateType
        {
                Quietly,
                WithMessage
        };

        void terminate(TerminateType terminate_type) noexcept
        {
                try
                {
                        switch (terminate_type)
                        {
                        case TerminateType::Quietly:
                                m_progress_list.terminate_all_quietly();
                                break;
                        case TerminateType::WithMessage:
                                m_progress_list.terminate_all_with_message();
                                break;
                        }

                        if (m_thread.joinable())
                        {
                                m_thread.join();
                        }

                        m_progress_list.enable();
                }
                catch (...)
                {
                        switch (terminate_type)
                        {
                        case TerminateType::Quietly:
                                error_fatal("Error terminating thread quietly error");
                        case TerminateType::WithMessage:
                                error_fatal("Error terminating thread with message error");
                        };
                }
        }

public:
        ThreadData(const std::function<void(const std::exception_ptr& ptr, const std::string& msg)>& exception_handler)
                : m_exception_handler(exception_handler)
        {
        }

        template <typename F>
        void start(F&& function)
        {
                terminate_quietly();

                ASSERT(!m_working);

                m_working = true;
                m_thread = std::thread([ this, func = std::forward<F>(function) ]() noexcept {
                        std::string message;
                        try
                        {
                                static_assert(!noexcept(func(&m_progress_list, &message)));

                                func(&m_progress_list, &message);
                        }
                        catch (...)
                        {
                                m_exception_handler(std::current_exception(), message);
                        }
                        m_working = false;
                });
        }

        void terminate_quietly() noexcept
        {
                terminate(TerminateType::Quietly);
        }

        void terminate_with_message() noexcept
        {
                terminate(TerminateType::WithMessage);
        }

        bool working() const noexcept
        {
                return m_working;
        }

        bool joinable() const noexcept
        {
                return m_thread.joinable();
        }

        const ProgressRatioList* progress_list() const noexcept
        {
                return &m_progress_list;
        }

        std::list<QProgressBar>* progress_bars() noexcept
        {
                return &m_progress_bars;
        }
};

//

class Impl final : public MainThreads
{
        const std::thread::id m_thread_id;
        const std::function<void(const std::exception_ptr& ptr, const std::string& msg)> m_exception_handler;

        std::unordered_map<Action, ThreadData> m_threads;
        std::vector<Progress> m_progress;

        ThreadData& thread(Action action)
        {
                auto t = m_threads.find(action);
                ASSERT(t != m_threads.end());
                return t->second;
        }

        const ThreadData& thread(Action action) const
        {
                auto t = m_threads.find(action);
                ASSERT(t != m_threads.end());
                return t->second;
        }

public:
        Impl(const std::function<void(const std::exception_ptr& ptr, const std::string& msg)>& exception_handler)
                : m_thread_id(std::this_thread::get_id()), m_exception_handler(exception_handler)
        {
                m_threads.try_emplace(Action::Load, m_exception_handler);
                m_threads.try_emplace(Action::Export, m_exception_handler);
                m_threads.try_emplace(Action::ReloadBoundCocone, m_exception_handler);
                m_threads.try_emplace(Action::SelfTest, m_exception_handler);

                for (auto& t : m_threads)
                {
                        Progress p;

                        p.action = t.first;
                        p.permanent = (t.first == Action::SelfTest);
                        p.progress_list = t.second.progress_list();
                        p.progress_bars = t.second.progress_bars();

                        m_progress.push_back(p);
                }
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                ASSERT(std::all_of(m_threads.cbegin(), m_threads.cend(),
                                   [](const auto& t) { return !t.second.working() && !t.second.joinable(); }));
        }

        void terminate_thread_with_message(Action action) noexcept override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                thread(action).terminate_with_message();
        }

        void terminate_all_threads() noexcept override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                for (auto& t : m_threads)
                {
                        t.second.terminate_quietly();
                }
        }

        bool action_allowed(Action action) const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                switch (action)
                {
                case Action::Load:
                        return true;
                case Action::Export:
                        return !thread(Action::Export).working();
                case Action::ReloadBoundCocone:
                        return !thread(Action::Load).working();
                case Action::SelfTest:
                        return true;
                }

                error_fatal("Unknown thread action");
        }

        void start_thread(Action action, const std::function<void(ProgressRatioList*, std::string*)>& function) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (!action_allowed(action))
                {
                        error_fatal("Thread action not allowed");
                }

                switch (action)
                {
                case Action::Load:
                        thread(Action::ReloadBoundCocone).terminate_quietly();
                        break;
                case Action::Export:
                        break;
                case Action::ReloadBoundCocone:
                        break;
                case Action::SelfTest:
                        break;
                }

                thread(action).start(function);
        }

        const std::vector<Progress>& thread_progress() const override
        {
                return m_progress;
        }
};
}

std::unique_ptr<MainThreads> create_main_threads(
        const std::function<void(const std::exception_ptr& ptr, const std::string& msg)>& exception_handler)
{
        return std::make_unique<Impl>(exception_handler);
}
