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

#include "time.h"

#include "error.h"

#include <atomic>
#include <chrono>
#include <thread>

using CLOCK = std::chrono::steady_clock;
// using CLOCK = std::chrono::high_resolution_clock;

namespace
{
std::atomic_int global_call_counter = 0;
CLOCK::time_point* global_start_time = nullptr;
}

void time_init()
{
        if (++global_call_counter != 1)
        {
                error_fatal("Error time init");
        }
        global_start_time = new CLOCK::time_point(CLOCK::now());
}

void time_exit()
{
        delete global_start_time;
        global_start_time = nullptr;
        --global_call_counter;
}

//

double time_in_seconds() noexcept
{
        try
        {
                CLOCK::time_point now = CLOCK::now();
                std::chrono::duration<double> time = now - *global_start_time;
                return time.count();
        }
        catch (...)
        {
                error_fatal("Exception in time function");
        }
}

void sleep_this_thread_until(double time) noexcept
{
        try
        {
                std::this_thread::sleep_until(*global_start_time + std::chrono::duration<double>(time));
        }
        catch (...)
        {
                error_fatal("Exception in sleep function");
        }
}
