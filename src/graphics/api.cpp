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

#include "api.h"

#include <src/com/error.h>

std::string to_string(GraphicsAndComputeAPI api)
{
        switch (api)
        {
        case GraphicsAndComputeAPI::Vulkan:
                return "Vulkan";
#if defined(OPENGL_FOUND)
        case GraphicsAndComputeAPI::OpenGL:
                return "OpenGL";
#endif
        }
        error_fatal("Unknown graphics and compute API");
}
