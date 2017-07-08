/*
Copyright (C) 2017 Topological Manifold

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

#include <memory>

// Для работы в потоках расчётов
class ProgressRatio
{
        class Impl;
        std::unique_ptr<Impl> m_progress;

public:
        static constexpr bool LOCK_FREE = (ATOMIC_LLONG_LOCK_FREE == 2) && (ATOMIC_BOOL_LOCK_FREE == 2);

        ProgressRatio(IProgressRatioList* list, const std::string& permanent_text = "");
        ~ProgressRatio();
        void set(unsigned v, unsigned m);
        void set(double v);
        void set_undefined();
        void set_text(const std::string& text);
};
