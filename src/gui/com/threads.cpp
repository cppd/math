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

#include "threads.h"

#include "../dialogs/message.h"

#include <src/com/exception.h>

#include <atomic>
#include <deque>
#include <thread>

namespace gui
{
namespace
{
class ThreadData
{
        ProgressRatioList m_progress_list;
        std::list<QProgressBar> m_progress_bars;
        std::thread m_thread;
        std::atomic_bool m_working = false;

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
                        }
                }
        }

public:
        template <typename F>
        void start(const std::string& description, F&& function)
        {
                terminate_quietly();

                ASSERT(!m_working);

                m_working = true;
                m_thread = std::thread([this, func = std::forward<F>(function), description]() noexcept {
                        try
                        {
                                catch_all(description, [&]() { func(&m_progress_list); });
                                m_working = false;
                        }
                        catch (...)
                        {
                                error_fatal("Exception in thread");
                        }
                });
        }

        void terminate_quietly()
        {
                terminate(TerminateType::Quietly);
        }

        void terminate_with_message()
        {
                terminate(TerminateType::WithMessage);
        }

        bool working() const
        {
                return m_working;
        }

        bool joinable() const
        {
                return m_thread.joinable();
        }

        const ProgressRatioList* progress_list() const
        {
                return &m_progress_list;
        }

        std::list<QProgressBar>* progress_bars()
        {
                return &m_progress_bars;
        }
};

//

class Impl final : public WorkerThreads
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        std::deque<ThreadData> m_threads;
        std::vector<Progress> m_progress;

        ThreadData& thread_data(unsigned id)
        {
                ASSERT(id < m_threads.size());
                return m_threads[id];
        }

        const ThreadData& thread_data(unsigned id) const
        {
                ASSERT(id < m_threads.size());
                return m_threads[id];
        }

        void start(unsigned id, const std::string& description, std::function<void(ProgressRatioList*)>&& function)
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (!function)
                {
                        return;
                }

                thread_data(id).start(description, std::move(function));
        }

        bool terminate_with_dialog(unsigned id)
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (is_working(id))
                {
                        bool yes;
                        if (!dialog::message_question_default_no(
                                    "There is work in progress.\nDo you want to continue?", &yes)
                            || !yes)
                        {
                                return false;
                        }
                }

                terminate_quietly(id);

                return true;
        }

        bool is_working(unsigned id) const
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                return thread_data(id).working();
        }

        void terminate_quietly(unsigned id)
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                thread_data(id).terminate_quietly();
        }

        void terminate_with_message(unsigned id) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                thread_data(id).terminate_with_message();
        }

        bool terminate_and_start(unsigned id, const std::string& description, std::function<Function()>&& function)
                override
        {
                bool result = false;
                catch_all(description, [&]() {
                        if (!terminate_with_dialog(id))
                        {
                                return;
                        }
                        start(id, description, function());
                        result = true;
                });
                return result;
        }

        void terminate_all() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                for (ThreadData& t : m_threads)
                {
                        t.terminate_quietly();
                }
        }

        const std::vector<Progress>& progresses() const override
        {
                return m_progress;
        }

        unsigned count() const override
        {
                return m_threads.size();
        }

public:
        Impl(unsigned thread_count)
        {
                ASSERT(thread_count > 0);

                m_threads.resize(thread_count);

                for (unsigned id = 0; id < m_threads.size(); ++id)
                {
                        Progress p;
                        p.id = id;
                        p.progress_list = m_threads[id].progress_list();
                        p.progress_bars = m_threads[id].progress_bars();
                        m_progress.push_back(p);
                }
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                if (std::any_of(m_threads.cbegin(), m_threads.cend(), [](const auto& t) {
                            return t.working() || t.joinable();
                    }))
                {
                        error_fatal("Working threads in the work thread class destructor");
                }
        }
};
}

std::unique_ptr<WorkerThreads> create_worker_threads(unsigned thread_count)
{
        return std::make_unique<Impl>(thread_count);
}
}
