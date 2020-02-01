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

#include "create.h"

#include "opengl/show.h"
#include "vulkan/show.h"

#include <src/com/error.h>

std::unique_ptr<ShowObject> create_show_object(GraphicsAndComputeAPI api, const ShowCreateInfo& info)
{
        switch (api)
        {
#if defined(OPENGL_FOUND)
        case GraphicsAndComputeAPI::OpenGL:
                return show_opengl::create_show_object(info);
#endif
        case GraphicsAndComputeAPI::Vulkan:
                return show_vulkan::create_show_object(info);
        }
        error("Unknown graphics and compute API for show creation");
}
