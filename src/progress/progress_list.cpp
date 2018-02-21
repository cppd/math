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

#include "progress_list.h"

// Для работы в потоках расчётов
void ProgressRatioList::add_progress_ratio(IProgressRatioControl* ratio)
{
        std::lock_guard lg(m_mutex);

        if (m_stop)
        {
                throw TerminateRequestException();
        }

        m_ratios.emplace_back(ratio);
}

// Для работы в потоках расчётов
void ProgressRatioList::delete_progress_ratio(const IProgressRatioControl* ratio)
{
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

// Для работы в потоке интерфейса
void ProgressRatioList::stop_all()
{
        std::lock_guard lg(m_mutex);

        m_stop = true;

        for (auto i = m_ratios.begin(); i != m_ratios.end(); ++i)
        {
                (*i)->set_terminate();
        }
}

// Для работы в потоке интерфейса
void ProgressRatioList::enable()
{
        std::lock_guard lg(m_mutex);

        ASSERT(m_ratios.size() == 0);

        m_stop = false;
}

// Для работы в потоке интерфейса
std::vector<std::tuple<unsigned, unsigned, std::string>> ProgressRatioList::get_all() const
{
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
