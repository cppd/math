/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "image_resolve.h"

#include "../event.h"

#include <src/gpu/convex_hull/view.h>
#include <src/gpu/dft/view.h>
#include <src/gpu/optical_flow/view.h>
#include <src/gpu/pencil_sketch/view.h>
#include <src/gpu/render_buffers.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/physical_device/functionality.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <memory>
#include <optional>
#include <vector>

namespace ns::view
{
class ImageProcess final
{
        bool convex_hull_active_ = false;
        bool pencil_sketch_active_ = false;
        bool dft_active_ = false;
        bool optical_flow_active_ = false;

        std::unique_ptr<gpu::convex_hull::View> convex_hull_;
        std::unique_ptr<gpu::pencil_sketch::View> pencil_sketch_;
        std::unique_ptr<gpu::dft::View> dft_;
        std::unique_ptr<gpu::optical_flow::View> optical_flow_;

        std::vector<std::array<vulkan::handle::Semaphore, 2>> resolve_semaphores_;

        void cmd(const command::PencilSketchShow& v);
        void cmd(const command::DftShow& v);
        void cmd(const command::DftSetBrightness& v);
        void cmd(const command::DftSetBackgroundColor& v);
        void cmd(const command::DftSetColor& v);
        void cmd(const command::ConvexHullShow& v);
        void cmd(const command::OpticalFlowShow& v);

public:
        static vulkan::DeviceFunctionality device_functionality();

        ImageProcess(
                bool sample_shading,
                const vulkan::Device* device,
                const vulkan::CommandPool* graphics_command_pool,
                const vulkan::Queue* graphics_queue,
                const vulkan::CommandPool* transfer_command_pool,
                const vulkan::Queue* transfer_queue,
                const vulkan::CommandPool* compute_command_pool,
                const vulkan::Queue* compute_queue,
                unsigned image_count);

        void exec(const ImageCommand& command);

        [[nodiscard]] bool two_windows() const;

        void delete_buffers();

        void create_buffers(
                double window_ppi,
                gpu::RenderBuffers2D* render_buffers,
                const vulkan::ImageWithMemory& input,
                const vulkan::ImageWithMemory& objects,
                const Region<2, int>& window_1,
                const std::optional<Region<2, int>>& window_2);

        [[nodiscard]] VkSemaphore draw(
                const ImageResolve& image_resolve,
                VkSemaphore semaphore,
                const vulkan::Queue& graphics_queue,
                const vulkan::Queue& compute_queue,
                unsigned image_index) const;
};
}
