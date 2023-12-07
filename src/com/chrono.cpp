/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "chrono.h"

#include <chrono>
#include <ctime>
#include <mutex>

namespace ns
{
std::tm time_to_local_time(const std::chrono::system_clock::time_point& time)
{
        const std::time_t t = std::chrono::system_clock::to_time_t(time);
        static std::mutex mutex;
        const std::lock_guard lg(mutex);
        return *std::localtime(&t);
}
}
