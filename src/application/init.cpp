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

#include "init.h"

#include <src/com/error.h>
#include <src/com/time.h>
#include <src/window/vulkan/window.h>

#include <atomic>

#if defined(__linux__)
#include <src/window/manage.h>
#endif

namespace application
{
namespace
{
std::atomic_int global_call_counter = 0;
}

Init::Init()
{
        if (++global_call_counter != 1)
        {
                error_fatal("Initialization must be called once");
        }

        time_init();

#if defined(__linux__)
        xlib_init();
#endif

#if 0
        vulkan::window_init();
#endif
}

Init::~Init()
{
#if 0
        vulkan::window_terminate();
#endif

#if defined(__linux__)
        xlib_exit();
#endif

        time_exit();
}
}
