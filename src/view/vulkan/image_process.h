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

#include "image_resolve.h"

#include "../event.h"

#include <src/gpu/convex_hull/view.h>
#include <src/gpu/dft/view.h>
#include <src/gpu/optical_flow/view.h>
#include <src/gpu/pencil_sketch/view.h>
#include <src/gpu/render_buffers.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/instance.h>

namespace ns::view
{
class ImageProcess
{
        const double window_ppi_;

        bool convex_hull_active_ = false;
        bool pencil_sketch_active_ = false;
        bool dft_active_ = false;
        bool optical_flow_active_ = false;

        std::unique_ptr<gpu::convex_hull::View> convex_hull_;
        std::unique_ptr<gpu::pencil_sketch::View> pencil_sketch_;
        std::unique_ptr<gpu::dft::View> dft_;
        std::unique_ptr<gpu::optical_flow::View> optical_flow_;

        void command(const command::PencilSketchShow& v);
        void command(const command::DftShow& v);
        void command(const command::DftSetBrightness& v);
        void command(const command::DftSetBackgroundColor& v);
        void command(const command::DftSetColor& v);
        void command(const command::ConvexHullShow& v);
        void command(const command::OpticalFlowShow& v);

public:
        static std::vector<std::string> required_device_extensions();
        static std::vector<std::string> optional_device_extensions();
        static vulkan::PhysicalDeviceFeatures required_device_features();
        static vulkan::PhysicalDeviceFeatures optional_device_features();

        ImageProcess(
                double window_ppi,
                bool sample_shading,
                const vulkan::VulkanInstance* instance,
                const vulkan::CommandPool* graphics_command_pool,
                const vulkan::Queue* graphics_queue,
                const vulkan::CommandPool* transfer_command_pool,
                const vulkan::Queue* transfer_queue,
                const vulkan::CommandPool* compute_command_pool,
                const vulkan::Queue* compute_queue);

        void command(const ImageCommand& image_command);

        bool two_windows() const;

        void delete_buffers();

        void create_buffers(
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
