/*
Copyright (C) 2017-2026 Topological Manifold

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

/*
Satyan L. Devadoss, Joseph O’Rourke.
Discrete and computational geometry.
Princeton University Press, 2011.

2 CONVEX HULLS
2.6 Divide-and-Conquer
*/

#include "compute.h"

#include "barrier.h"
#include "size.h"

#include "shaders/filter.h"
#include "shaders/merge.h"
#include "shaders/prepare.h"

#include <src/com/error.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

namespace ns::gpu::convex_hull
{
namespace
{
int group_size_merge(const int height, const VkPhysicalDeviceLimits& limits)
{
        return convex_hull::group_size_merge(
                height, limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupInvocations,
                limits.maxComputeSharedMemorySize);
}

int group_size_prepare(const int width, const VkPhysicalDeviceLimits& limits)
{
        return convex_hull::group_size_prepare(
                width, limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupInvocations,
                limits.maxComputeSharedMemorySize);
}

class Impl final : public Compute
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        const vulkan::Device* const device_;

        std::optional<vulkan::BufferWithMemory> lines_buffer_;

        unsigned prepare_group_count_ = 0;
        PrepareProgram prepare_program_;
        PrepareMemory prepare_memory_;

        MergeProgram merge_program_;
        MergeMemory merge_memory_;

        FilterProgram filter_program_;
        FilterMemory filter_memory_;

        void compute_commands(const VkCommandBuffer command_buffer) const override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                ASSERT(lines_buffer_);

                //

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, prepare_program_.pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, prepare_program_.pipeline_layout(),
                        PrepareMemory::set_number(), 1, &prepare_memory_.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, prepare_group_count_, 1, 1);

                //

                buffer_barrier(
                        command_buffer, lines_buffer_->buffer().handle(), VK_ACCESS_SHADER_READ_BIT,
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

                //

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, merge_program_.pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, merge_program_.pipeline_layout(),
                        MergeMemory::set_number(), 1, &merge_memory_.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, 2, 1, 1);

                //

                buffer_barrier(
                        command_buffer, lines_buffer_->buffer().handle(), VK_ACCESS_SHADER_READ_BIT,
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

                //

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, filter_program_.pipeline());
                vkCmdBindDescriptorSets(
                        command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, filter_program_.pipeline_layout(),
                        FilterMemory::set_number(), 1, &filter_memory_.descriptor_set(), 0, nullptr);
                vkCmdDispatch(command_buffer, 1, 1, 1);
        }

        void create_buffers(
                const vulkan::ImageWithMemory& objects,
                const numerical::Region<2, int>& rectangle,
                const vulkan::Buffer& points_buffer,
                const vulkan::Buffer& point_count_buffer,
                const std::uint32_t family_index) override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                ASSERT(rectangle.is_positive());
                ASSERT(objects.image().type() == VK_IMAGE_TYPE_2D);
                ASSERT(rectangle.x1() <= static_cast<int>(objects.image().extent().width));
                ASSERT(rectangle.y1() <= static_cast<int>(objects.image().extent().height));

                ASSERT(points_buffer.size() == (2 * rectangle.height() + 1) * (2 * sizeof(std::int32_t)));
                ASSERT(point_count_buffer.size() >= sizeof(std::int32_t));

                const int width = rectangle.width();
                const int height = rectangle.height();

                lines_buffer_.emplace(
                        vulkan::BufferMemoryType::DEVICE_LOCAL, *device_, std::vector({family_index}),
                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, sizeof(std::int32_t) * 2 * height);

                prepare_memory_.set_object_image(objects.image_view());
                prepare_memory_.set_lines(lines_buffer_->buffer());
                prepare_group_count_ = height;
                prepare_program_.create_pipeline(
                        group_size_prepare(width, device_->properties().properties_10.limits), rectangle);

                merge_memory_.set_lines(lines_buffer_->buffer());
                merge_program_.create_pipeline(
                        height, group_size_merge(height, device_->properties().properties_10.limits),
                        iteration_count_merge(height));

                filter_memory_.set_lines(lines_buffer_->buffer());
                filter_memory_.set_points(points_buffer);
                filter_memory_.set_point_count(point_count_buffer);
                filter_program_.create_pipeline(height);
        }

        void delete_buffers() override
        {
                ASSERT(thread_id_ == std::this_thread::get_id());

                //

                filter_program_.delete_pipeline();
                merge_program_.delete_pipeline();
                prepare_program_.delete_pipeline();
                prepare_group_count_ = 0;

                lines_buffer_.reset();
        }

public:
        explicit Impl(const vulkan::Device* const device)
                : device_(device),
                  prepare_program_(device_->handle()),
                  prepare_memory_(device_->handle(), prepare_program_.descriptor_set_layout()),
                  merge_program_(device_->handle()),
                  merge_memory_(device_->handle(), merge_program_.descriptor_set_layout()),
                  filter_program_(device_->handle()),
                  filter_memory_(device_->handle(), filter_program_.descriptor_set_layout())
        {
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == thread_id_);

                device_->wait_idle_noexcept("convex hull compute destructor");
        }
};
}

std::unique_ptr<Compute> create_compute(const vulkan::Device* const device)
{
        return std::make_unique<Impl>(device);
}
}
