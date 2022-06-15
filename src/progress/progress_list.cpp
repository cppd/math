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

#include "progress_list.h"

#include <src/com/error.h>
#include <src/com/exception.h>

#include <thread>

namespace ns::progress
{
// for worker threads
void RatioList::add_progress_ratio(RatioControl* const ratio)
{
        ASSERT(std::this_thread::get_id() != thread_id_);

        std::lock_guard lg(mutex_);

        if (terminate_quietly_)
        {
                throw TerminateQuietlyException();
        }

        if (terminate_with_message_)
        {
                throw TerminateWithMessageException();
        }

        ratios_.emplace_back(ratio);
}

// for worker threads
void RatioList::delete_progress_ratio(const RatioControl* const ratio) noexcept
{
        try
        {
                ASSERT(std::this_thread::get_id() != thread_id_);

                std::lock_guard lg(mutex_);

                for (auto i = ratios_.begin(); i != ratios_.end(); ++i)
                {
                        if (*i == ratio)
                        {
                                ratios_.erase(i);
                                return;
                        }
                }
        }
        catch (...)
        {
                error_fatal("Exception in delete progress ratio");
        }
}

// for UI thread
void RatioList::terminate_all_quietly()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        std::lock_guard lg(mutex_);

        terminate_quietly_ = true;
        for (RatioControl* const ratio : ratios_)
        {
                ratio->terminate_quietly();
        }
}

// for UI thread
void RatioList::terminate_all_with_message()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        std::lock_guard lg(mutex_);

        terminate_with_message_ = true;
        for (RatioControl* const ratio : ratios_)
        {
                ratio->terminate_with_message();
        }
}

// for UI thread
void RatioList::enable()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        std::lock_guard lg(mutex_);

        ASSERT(ratios_.empty());

        terminate_quietly_ = false;
        terminate_with_message_ = false;
}

// for UI thread
std::vector<std::tuple<unsigned, unsigned, std::string>> RatioList::ratios() const
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        std::lock_guard lg(mutex_);

        std::vector<std::tuple<unsigned, unsigned, std::string>> result;
        result.reserve(ratios_.size());

        for (const RatioControl* const ratio : ratios_)
        {
                const RatioControl::Info info = ratio->info();
                result.emplace_back(info.value, info.maximum, ratio->text());
        }
        return result;
}
}
