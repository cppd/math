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

#pragma once

#include "buffer.h"

#include <src/vulkan/device.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace ns::gpu::dft
{
class Fft
{
public:
        virtual ~Fft() = default;

        virtual void set_data(const ComplexNumberBuffer& data) = 0;

        virtual void commands(VkCommandBuffer command_buffer, bool inverse) const = 0;

        virtual void run_for_data(
                bool inverse,
                const ComplexNumberBuffer& data,
                VkDevice device,
                VkCommandPool pool,
                VkQueue queue) = 0;
};

std::unique_ptr<Fft> create_fft(
        const vulkan::Device& device,
        const std::vector<std::uint32_t>& family_indices,
        unsigned count,
        unsigned n);
}
