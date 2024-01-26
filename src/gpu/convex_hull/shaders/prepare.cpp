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

#include "prepare.h"

#include <src/com/error.h>
#include <src/gpu/convex_hull/code/code.h>
#include <src/numerical/region.h>
#include <src/vulkan/create.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/pipeline/compute.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ns::gpu::convex_hull
{
namespace
{
class SpecializationConstants final
{
        struct Data final
        {
                std::int32_t local_size_x;
                std::int32_t buffer_size;
                std::int32_t x;
                std::int32_t y;
                std::int32_t width;
                std::int32_t height;
        };

        static constexpr std::array<VkSpecializationMapEntry, 6> ENTRIES{
                {{0, offsetof(Data, local_size_x), sizeof(Data::local_size_x)},
                 {1, offsetof(Data, buffer_size), sizeof(Data::buffer_size)},
                 {2, offsetof(Data, x), sizeof(Data::x)},
                 {3, offsetof(Data, y), sizeof(Data::y)},
                 {4, offsetof(Data, width), sizeof(Data::width)},
                 {5, offsetof(Data, height), sizeof(Data::height)}}
        };

        Data data_;

        VkSpecializationInfo info_{
                .mapEntryCount = ENTRIES.size(),
                .pMapEntries = ENTRIES.data(),
                .dataSize = sizeof(data_),
                .pData = &data_};

public:
        SpecializationConstants(
                const std::int32_t local_size_x,
                const std::int32_t buffer_size,
                const numerical::Region<2, int>& rectangle)
                : data_{.local_size_x = local_size_x,
                        .buffer_size = buffer_size,
                        .x = rectangle.x0(),
                        .y = rectangle.y0(),
                        .width = rectangle.width(),
                        .height = rectangle.height()}
        {
                ASSERT(rectangle.is_positive());
        }

        SpecializationConstants(const SpecializationConstants&) = delete;
        SpecializationConstants& operator=(const SpecializationConstants&) = delete;

        [[nodiscard]] const VkSpecializationInfo& info() const
        {
                return info_;
        }
};
}

std::vector<VkDescriptorSetLayoutBinding> PrepareMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(2);

        bindings.push_back(
                {.binding = OBJECTS_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = LINES_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                 .pImmutableSamplers = nullptr});

        return bindings;
}

PrepareMemory::PrepareMemory(const VkDevice device, const VkDescriptorSetLayout descriptor_set_layout)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
}

unsigned PrepareMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& PrepareMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

void PrepareMemory::set_object_image(const vulkan::ImageView& storage_image) const
{
        ASSERT(storage_image.format() == VK_FORMAT_R32_UINT);
        ASSERT(storage_image.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));

        descriptors_.update_descriptor_set(
                0, OBJECTS_BINDING,
                VkDescriptorImageInfo{
                        .sampler = VK_NULL_HANDLE,
                        .imageView = storage_image.handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL});
}

void PrepareMemory::set_lines(const vulkan::Buffer& buffer) const
{
        ASSERT(buffer.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        descriptors_.update_descriptor_set(
                0, LINES_BINDING,
                VkDescriptorBufferInfo{.buffer = buffer.handle(), .offset = 0, .range = buffer.size()});
}

//

PrepareProgram::PrepareProgram(const VkDevice device)
        : device_(device),
          descriptor_set_layout_(
                  vulkan::create_descriptor_set_layout(device, PrepareMemory::descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {PrepareMemory::set_number()}, {descriptor_set_layout_})),
          shader_(device, code_prepare_comp(), VK_SHADER_STAGE_COMPUTE_BIT)
{
}

void PrepareProgram::create_pipeline(const unsigned buffer_and_group_size, const numerical::Region<2, int>& rectangle)
{
        const SpecializationConstants constants(buffer_and_group_size, buffer_and_group_size, rectangle);

        vulkan::ComputePipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shader = &shader_;
        info.constants = &constants.info();
        pipeline_ = create_compute_pipeline(info);
}

void PrepareProgram::delete_pipeline()
{
        pipeline_ = vulkan::handle::Pipeline();
}

VkDescriptorSetLayout PrepareProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout PrepareProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

VkPipeline PrepareProgram::pipeline() const
{
        return pipeline_;
}
}
