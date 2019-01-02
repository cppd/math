/*
Copyright (C) 2017-2019 Topological Manifold

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
#include <string>

// Для работы в потоках расчётов
class ProgressRatio
{
        class Impl;
        std::unique_ptr<Impl> m_progress;

public:
        ProgressRatio(ProgressRatios* ratios, const std::string& permanent_text = "");
        ~ProgressRatio();

        void set(unsigned v, unsigned m);
        void set(double v);
        void set_undefined();
        void set_text(const std::string& text);

        static bool lock_free();
};
