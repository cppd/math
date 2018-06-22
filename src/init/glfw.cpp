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

#include "glfw.h"

#if defined(GLFW_FOUND)

#include "com/error.h"
#include "com/log.h"

#include <GLFW/glfw3.h>
#include <string>

namespace
{
void glfw_error_callback(int /*error*/, const char* description)
{
        LOG(std::string("GLFW Error: ") + description);
}
}

void glfw_init()
{
        glfwSetErrorCallback(glfw_error_callback);

        if (glfwInit() != GLFW_TRUE)
        {
                error("Failed to initialize GLFW");
        }
}

void glfw_terminate() noexcept
{
        glfwTerminate();
}

#else

void glfw_init()
{
}

void glfw_terminate() noexcept
{
}

#endif
