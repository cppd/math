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

#pragma once

#include "progress/progress_interfaces.h"

#include <exception>
#include <functional>

enum class SelfTestType
{
        Essential,
        Extended
};

void self_test(SelfTestType test_type, ProgressRatios* progress_ratios,
               std::function<void(const std::exception_ptr& ptr, const std::string& msg)> exception_handler);
