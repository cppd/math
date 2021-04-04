/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "progress_interfaces.h"

#include <list>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

namespace ns
{
class ProgressRatioList final : public ProgressRatios
{
        const std::thread::id m_thread_id = std::this_thread::get_id();
        std::list<ProgressRatioControl*> m_ratios;
        bool m_terminate_quietly = false;
        bool m_terminate_with_message = false;
        mutable std::mutex m_mutex;

public:
        // Для работы в потоках расчётов
        void add_progress_ratio(ProgressRatioControl* ratio) override;
        void delete_progress_ratio(const ProgressRatioControl* ratio) noexcept override;

        // Для работы в потоке интерфейса
        void terminate_all_quietly();
        void terminate_all_with_message();
        void enable();
        std::vector<std::tuple<unsigned, unsigned, std::string>> ratios() const;
};
}
