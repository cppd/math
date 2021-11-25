/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "image_type.h"

#include <src/com/enum.h>
#include <src/com/print.h>

namespace ns::vulkan
{
std::string image_type_to_string(const VkImageType image_type)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        switch (image_type)
        {
        case VK_IMAGE_TYPE_1D:
                return "VK_IMAGE_TYPE_1D";
        case VK_IMAGE_TYPE_2D:
                return "VK_IMAGE_TYPE_2D";
        case VK_IMAGE_TYPE_3D:
                return "VK_IMAGE_TYPE_3D";
        }
#pragma GCC diagnostic pop

        return "Unknown VkImageType " + to_string(enum_to_int(image_type));
}
}
