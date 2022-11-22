/*
Copyright (C) 2017-2022 Topological Manifold

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

#pragma once

#include <memory>
#include <vulkan/vulkan.h>

namespace ns::vulkan
{
class Instance final
{
        class Impl;
        std::unique_ptr<Impl> instance_;

        inline static VkInstance instance_handle_ = VK_NULL_HANDLE;

public:
        Instance();
        ~Instance();

        Instance(const Instance&) = delete;
        Instance& operator=(const Instance&) = delete;

        static VkInstance handle()
        {
                return instance_handle_;
        }
};
}
