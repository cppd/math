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

#if defined(VULKAN_FOUND)

#include "test_vulkan.h"

#include "com/log.h"
#include "graphics/vulkan/objects.h"

#include <memory>

void test_vulkan()
{
        const std::vector<const char*> extensions({});
        const std::vector<const char*> layers({"VK_LAYER_LUNARG_standard_validation"});

        LOG(vulkan::overview());

        vulkan::VulkanInstance instance(1, 0, extensions, layers);

        LOG(vulkan::overview_physical_devices(instance));
}

#endif
