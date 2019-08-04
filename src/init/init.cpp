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

#include "init.h"

#include "com/error.h"
#include "com/log.h"
#include "com/time.h"
#include "window/opengl/window.h"
#include "window/vulkan/window.h"

#include <atomic>

#if defined(__linux__)
#include "graphics/opengl/functions/opengl_functions.h"
#include "window/manage.h"
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

        reset_time();

#if defined(__linux__)

        xlib_init();

        // Для Линукса адреса функций OpenGL не зависят от контекста,
        // поэтому их можно определять один раз при запуске программы.
        opengl_functions::init();

#endif

        vulkan::window_init();

        opengl::window_init();
}

Initialization::~Initialization()
{
        vulkan::window_terminate();

        log_exit();
}
