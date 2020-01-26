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

#include "progress_list.h"

#include "com/error.h"

// Для работы в потоках расчётов
void ProgressRatioList::add_progress_ratio(ProgressRatioControl* ratio)
{
        ASSERT(std::this_thread::get_id() != m_thread_id);

        std::lock_guard lg(m_mutex);

        if (m_terminate_quietly)
        {
                throw_terminate_quietly_exception();
        }

        if (m_terminate_with_message)
        {
                throw_terminate_with_message_exception();
        }

        m_ratios.emplace_back(ratio);
}

// Для работы в потоках расчётов
void ProgressRatioList::delete_progress_ratio(const ProgressRatioControl* ratio) noexcept
{
        try
        {
                ASSERT(std::this_thread::get_id() != m_thread_id);

                std::lock_guard lg(m_mutex);

                for (auto i = m_ratios.begin(); i != m_ratios.end(); ++i)
                {
                        if (*i == ratio)
                        {
                                m_ratios.erase(i);
                                return;
                        }
                }
        }
        catch (...)
        {
                error_fatal("Exception in delete progress ratio");
        }
}

// Для работы в потоке интерфейса
void ProgressRatioList::terminate_all_quietly()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::lock_guard lg(m_mutex);

        m_terminate_quietly = true;
        for (ProgressRatioControl* ratio : m_ratios)
        {
                ratio->terminate_quietly();
        }
}

// Для работы в потоке интерфейса
void ProgressRatioList::terminate_all_with_message()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::lock_guard lg(m_mutex);

        m_terminate_with_message = true;
        for (ProgressRatioControl* ratio : m_ratios)
        {
                ratio->terminate_with_message();
        }
}

// Для работы в потоке интерфейса
void ProgressRatioList::enable()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::lock_guard lg(m_mutex);

        ASSERT(m_ratios.empty());

        m_terminate_quietly = false;
        m_terminate_with_message = false;
}

// Для работы в потоке интерфейса
std::vector<std::tuple<unsigned, unsigned, std::string>> ProgressRatioList::ratios() const
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        std::lock_guard lg(m_mutex);

        std::vector<std::tuple<unsigned, unsigned, std::string>> result;
        result.reserve(m_ratios.size());

        for (auto i = m_ratios.cbegin(); i != m_ratios.cend(); ++i)
        {
                unsigned v, m;
                (*i)->get(&v, &m);
                result.emplace_back(v, m, (*i)->text());
        }
        return result;
}
