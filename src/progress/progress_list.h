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

#include "progress_interfaces.h"

#include "com/thread.h"

#include <list>
#include <string>
#include <tuple>
#include <vector>

class ProgressRatioList final : public IProgressRatioList
{
        std::list<IProgressRatioControl*> m_ratios;
        bool m_stop = false;
        mutable std::mutex m_mutex;

public:
        // Для работы в потоках расчётов
        void add_progress_ratio(IProgressRatioControl* ratio) override;
        void delete_progress_ratio(const IProgressRatioControl* ratio) override;

        // Для работы в потоке интерфейса
        void stop_all();
        void enable();
        std::vector<std::tuple<unsigned, unsigned, std::string>> ratios() const;
};
