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

// Generated by program

// clang-format off

#include "opengl_functions_proc.h"

#if defined(__linux__)

#include <GL/glx.h>

opengl_functions::PTR opengl_functions::gl_proc_address(const char* str)
{
        return glXGetProcAddress(reinterpret_cast<const GLubyte*>(str));
}

#elif defined(_WIN32)

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

opengl_functions::PTR opengl_functions::gl_proc_address(const char* str)
{
        opengl_functions::PTR ptr;
        ptr = reinterpret_cast<opengl_functions::PTR>(wglGetProcAddress(str));
        if (!ptr)
        {
                HMODULE module = LoadLibraryA("opengl32.dll");
                ptr = reinterpret_cast<opengl_functions::PTR>(GetProcAddress(module, str));
                FreeLibrary(module);
        }
        return ptr;
}

#else
#error This operating system is not supported
#endif

// clang-format on
