/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "support.h"

#include "../dialogs/message.h"

#include <src/com/error.h>
#include <src/com/exception.h>
#include <src/com/type/limit.h>

#include <QMenu>
#include <atomic>
#include <deque>
#include <thread>

namespace ns::gui
{
namespace
{
class ThreadData
{
        ProgressRatioList progress_list_;
        std::list<QProgressBar> progress_bars_;
        std::thread thread_;
        std::atomic_bool working_ = false;

        enum class TerminateType
        {
                QUIETLY,
                WITH_MESSAGE
        };

        void terminate(const TerminateType terminate_type) noexcept
        {
                try
                {
                        switch (terminate_type)
                        {
                        case TerminateType::QUIETLY:
                                progress_list_.terminate_all_quietly();
                                break;
                        case TerminateType::WITH_MESSAGE:
                                progress_list_.terminate_all_with_message();
                                break;
                        }

                        if (thread_.joinable())
                        {
                                thread_.join();
                        }

                        progress_list_.enable();
                }
                catch (...)
                {
                        switch (terminate_type)
                        {
                        case TerminateType::QUIETLY:
                                error_fatal("Error terminating thread quietly error");
                        case TerminateType::WITH_MESSAGE:
                                error_fatal("Error terminating thread with message error");
                        }
                }
        }

public:
        template <typename F>
        void start(const std::string& description, F&& function)
        {
                terminate_quietly();

                ASSERT(!working_);

                working_ = true;
                thread_ = std::thread(
                        [this, func = std::forward<F>(function), description]() noexcept
                        {
                                try
                                {
                                        catch_all(
                                                description,
                                                [&]()
                                                {
                                                        func(&progress_list_);
                                                });
                                        working_ = false;
                                }
                                catch (...)
                                {
                                        error_fatal("Exception in thread");
                                }
                        });
        }

        void terminate_quietly()
        {
                terminate(TerminateType::QUIETLY);
        }

        void terminate_with_message()
        {
                terminate(TerminateType::WITH_MESSAGE);
        }

        bool working() const
        {
                return working_;
        }

        bool joinable() const
        {
                return thread_.joinable();
        }

        const ProgressRatioList* progress_list() const
        {
                return &progress_list_;
        }

        std::list<QProgressBar>* progress_bars()
        {
                return &progress_bars_;
        }
};

//

class Impl final : public WorkerThreads
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        const std::optional<unsigned> permanent_thread_id_;
        QStatusBar* const status_bar_;

        std::deque<ThreadData> threads_;
        std::vector<Progress> progress_;

        ThreadData& thread_data(const unsigned id)
        {
                ASSERT(id < threads_.size());
                return threads_[id];
        }

        const ThreadData& thread_data(const unsigned id) const
        {
                ASSERT(id < threads_.size());
                return threads_[id];
        }

        void start(
                const unsigned id,
                const std::string& description,
                std::function<void(ProgressRatioList*)>&& function)
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                if (!function)
                {
                        return;
                }

                thread_data(id).start(description, std::move(function));
        }

        bool terminate_with_dialog(const unsigned id)
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                if (is_working(id))
                {
                        std::optional<bool> yes = dialog::message_question_default_no(
                                "There is work in progress.\nDo you want to continue?");
                        if (!yes || !*yes)
                        {
                                return false;
                        }
                }

                terminate_quietly(id);

                return true;
        }

        bool is_working(const unsigned id) const
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                return thread_data(id).working();
        }

        void terminate_quietly(const unsigned id)
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                thread_data(id).terminate_quietly();
        }

        void terminate_with_message(const unsigned id) override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                thread_data(id).terminate_with_message();
        }

        bool terminate_and_start(
                const unsigned id,
                const std::string& description,
                std::function<Function()>&& function) override
        {
                bool result = false;
                catch_all(
                        description,
                        [&]()
                        {
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
                ASSERT(std::this_thread::get_id() == thread_id_);

                for (ThreadData& t : threads_)
                {
                        t.terminate_quietly();
                }
        }

        unsigned count() const override
        {
                return threads_.size();
        }

        void set_progress(
                const unsigned id,
                const ProgressRatioList* const progress_list,
                std::list<QProgressBar>* const progress_bars)
        {
                constexpr unsigned MAX_INT{Limits<int>::max()};

                const std::vector<std::tuple<unsigned, unsigned, std::string>> ratios = progress_list->ratios();

                while (ratios.size() > progress_bars->size())
                {
                        QProgressBar& bar = progress_bars->emplace_back();

                        bar.setContextMenuPolicy(Qt::CustomContextMenu);

                        QObject::connect(
                                &bar, &QProgressBar::customContextMenuRequested,
                                [this, id, bar_ptr = QPointer(&bar)](const QPoint&)
                                {
                                        QtObjectInDynamicMemory<QMenu> menu(bar_ptr);
                                        menu->addAction("Terminate");

                                        if (menu->exec(QCursor::pos()) == nullptr || menu.isNull() || bar_ptr.isNull())
                                        {
                                                return;
                                        }

                                        terminate_with_message(id);
                                });
                }

                auto bar = progress_bars->begin();

                for (unsigned i = 0; i < ratios.size(); ++i, ++bar)
                {
                        if (!bar->isVisible())
                        {
                                if (id == permanent_thread_id_)
                                {
                                        status_bar_->addPermanentWidget(&(*bar));
                                }
                                else
                                {
                                        status_bar_->addWidget(&(*bar));
                                }
                                bar->show();
                        }

                        bar->setFormat(QString::fromStdString(std::get<2>(ratios[i])));

                        const unsigned maximum = std::get<1>(ratios[i]);

                        if (maximum > 0)
                        {
                                const unsigned value = std::min(maximum, std::get<0>(ratios[i]));

                                if (maximum <= MAX_INT)
                                {
                                        bar->setMaximum(maximum);
                                        bar->setValue(value);
                                }
                                else
                                {
                                        bar->setMaximum(MAX_INT);
                                        bar->setValue(std::lround((static_cast<double>(value) / maximum) * MAX_INT));
                                }
                        }
                        else
                        {
                                bar->setMaximum(0);
                                bar->setValue(0);
                        }
                }

                while (bar != progress_bars->end())
                {
                        status_bar_->removeWidget(&(*bar));
                        bar = progress_bars->erase(bar);
                }
        }

        void set_progresses() override
        {
                for (const WorkerThreads::Progress& t : progress_)
                {
                        set_progress(t.id, t.progress_list, t.progress_bars);
                }
        }

public:
        Impl(const unsigned thread_count,
             const std::optional<unsigned>& permanent_thread_id,
             QStatusBar* const status_bar)
                : permanent_thread_id_(permanent_thread_id), status_bar_(status_bar)
        {
                ASSERT(thread_count > 0);

                threads_.resize(thread_count);
                progress_.reserve(thread_count);

                for (unsigned id = 0; id < threads_.size(); ++id)
                {
                        Progress p;
                        p.id = id;
                        p.progress_list = threads_[id].progress_list();
                        p.progress_bars = threads_[id].progress_bars();
                        progress_.push_back(p);
                }
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                if (std::any_of(
                            threads_.cbegin(), threads_.cend(),
                            [](const auto& t)
                            {
                                    return t.working() || t.joinable();
                            }))
                {
                        error_fatal("Working threads in the work thread class destructor");
                }
        }
};
}

std::unique_ptr<WorkerThreads> create_worker_threads(
        const unsigned thread_count,
        const std::optional<unsigned>& permanent_thread_id,
        QStatusBar* const status_bar)
{
        return std::make_unique<Impl>(thread_count, permanent_thread_id, status_bar);
}
}
