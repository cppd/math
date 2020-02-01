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
#include <src/com/log.h>
#include <src/com/time.h>
#include <src/window/opengl/window.h>
#include <src/window/vulkan/window.h>

#include <atomic>

#if defined(__linux__)
#include <src/graphics/opengl/functions.h>
#include <src/window/manage.h>
#endif

namespace
{
std::atomic_int global_call_counter = 0;
}

Initialization::Initialization()
{
        if (++global_call_counter != 1)
        {
                error_fatal("Initialization must be called once");
        }

        log_init();
        time_init();

#if defined(__linux__)

        xlib_init();

#if defined(OPENGL_FOUND)
        // Для Линукса адреса функций OpenGL не зависят от контекста,
        // поэтому их можно определять один раз при запуске программы.
        opengl_functions::init();
#endif

#endif

#if 0
        vulkan::window_init();
#endif

#if defined(OPENGL_FOUND)
        opengl::window_init();
#endif
}

Initialization::~Initialization()
{
#if 0
        vulkan::window_terminate();
#endif

#if defined(__linux__)
        xlib_exit();
#endif

        time_exit();
        log_exit();
}
