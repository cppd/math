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

#include "init.h"

#include "com/error.h"
#include "com/log.h"
#include "com/time.h"

#include <atomic>

#if defined(__linux__)

#include "graphics/opengl/opengl_functions.h"

#include <X11/Xlib.h>
#include <cstdlib>

namespace
{
int X_error_handler(Display*, XErrorEvent* e)
{
        constexpr int BUF_SIZE = 1000;
        char buf[BUF_SIZE];
        XGetErrorText(e->display, e->error_code, buf, BUF_SIZE);
        error_fatal("X error handler: " + std::string(buf));
}

void init_XLib()
{
        if (!XInitThreads())
        {
                error_fatal("Error XInitThreads");
        }
        XSetErrorHandler(X_error_handler);
}

void init_gl()
{
        // Для Линукса адреса функций OpenGL не зависят от контекста,
        // поэтому их можно определять один раз при запуске программы.
        opengl_functions::init();
}

void init_os_specific()
{
        init_XLib();
        init_gl();
}
}

#elif defined(_WIN32)

namespace
{
void init_os_specific()
{
}
}

#else

#error This operating system is not supported

#endif

namespace
{
std::atomic_int global_call_counter = 0;
}

Initialization::Initialization()
{
        if (++global_call_counter > 1)
        {
                error_fatal("Initialization must be called once");
        }

        log_init();

        reset_time();

        init_os_specific();
}
Initialization::~Initialization()
{
        log_exit();
}
