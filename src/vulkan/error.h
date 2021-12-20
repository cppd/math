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

#pragma once

#include <type_traits>
#include <vulkan/vulkan.h>

namespace ns::vulkan::error_implementation
{
[[noreturn]] void vulkan_function_error(VkResult code);
[[noreturn]] void vulkan_function_error(VkResult code, const char* file, int line);

template <typename T>
void check_code(const T code)
{
        static_assert(std::is_same_v<T, VkResult>);
        if (code == VK_SUCCESS)
        {
                return;
        }
        vulkan_function_error(code);
}

template <typename T>
void check_code(const T code, const char* const file, const int line)
{
        static_assert(std::is_same_v<T, VkResult>);
        if (code == VK_SUCCESS)
        {
                return;
        }
        vulkan_function_error(code, file, line);
}

template <typename T>
[[noreturn]] void error_code(const T code)
{
        static_assert(std::is_same_v<T, VkResult>);
        vulkan_function_error(code);
}

template <typename T>
[[noreturn]] void error_code(const T code, const char* const file, const int line)
{
        static_assert(std::is_same_v<T, VkResult>);
        vulkan_function_error(code, file, line);
}
}

#if !defined(RELEASE_BUILD)
#define VULKAN_CHECK(code) ns::vulkan::error_implementation::check_code((code), __FILE__, __LINE__)
#else
#define VULKAN_CHECK(code) ns::vulkan::error_implementation::check_code((code))
#endif

#if !defined(RELEASE_BUILD)
#define VULKAN_ERROR(code) ns::vulkan::error_implementation::error_code((code), __FILE__, __LINE__)
#else
#define VULKAN_CHECK(code) ns::vulkan::error_implementation::error_code((code))
#endif
