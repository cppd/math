/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "functionality.h"

#include "features.h"

#include <string>

namespace ns::vulkan
{
void DeviceFunctionality::merge(const DeviceFunctionality& functionality)
{
        for (const std::string& extension : functionality.required_extensions)
        {
                required_extensions.insert(extension);
        }

        for (const std::string& extension : functionality.optional_extensions)
        {
                optional_extensions.insert(extension);
        }

        add_features(&required_features, functionality.required_features);

        add_features(&optional_features, functionality.optional_features);
}
}
