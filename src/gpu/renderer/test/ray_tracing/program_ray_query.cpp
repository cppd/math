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

#include "program_ray_query.h"

#include "descriptors.h"

#include "code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/error.h>
#include <src/vulkan/pipeline_compute.h>

namespace ns::gpu::renderer::test
{
namespace
{
class Constant final
{
        struct Data final
        {
                std::int32_t local_size_x;
                std::int32_t local_size_y;
        } data_;

        std::vector<VkSpecializationMapEntry> entries_;

public:
        Constant();

        void set(std::int32_t size);

        [[nodiscard]] VkSpecializationInfo info() const;
};

Constant::Constant()
{
        entries_.resize(2);

        entries_[0].constantID = 0;
        entries_[0].offset = offsetof(Data, local_size_x);
        entries_[0].size = sizeof(Data::local_size_x);

        entries_[1].constantID = 1;
        entries_[1].offset = offsetof(Data, local_size_y);
        entries_[1].size = sizeof(Data::local_size_y);
}

void Constant::set(const std::int32_t size)
{
        data_ = {.local_size_x = size, .local_size_y = size};
}

VkSpecializationInfo Constant::info() const
{
        VkSpecializationInfo info = {};
        info.mapEntryCount = entries_.size();
        info.pMapEntries = entries_.data();
        info.dataSize = sizeof(data_);
        info.pData = &data_;
        return info;
}
}

std::vector<VkDescriptorSetLayoutBinding> RayQueryProgram::descriptor_set_layout_bindings()
{
        constexpr bool RAYGEN = false;
        return RayTracingMemory::descriptor_set_layout_bindings(RAYGEN);
}

RayQueryProgram::RayQueryProgram(const VkDevice device, const unsigned local_size)
        : descriptor_set_layout_(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {RayTracingMemory::set_number()}, {descriptor_set_layout_}))
{
        const vulkan::Shader shader(device, code_ray_query_comp(), VK_SHADER_STAGE_COMPUTE_BIT);

        Constant constant;
        constant.set(local_size);

        const VkSpecializationInfo constant_info = constant.info();

        vulkan::ComputePipelineCreateInfo compute_info;
        compute_info.device = device;
        compute_info.pipeline_layout = pipeline_layout_;
        compute_info.shader = &shader;
        compute_info.constants = &constant_info;
        pipeline_ = vulkan::create_compute_pipeline(compute_info);
}

VkDescriptorSetLayout RayQueryProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout RayQueryProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

VkPipeline RayQueryProgram::pipeline() const
{
        return pipeline_;
}
}
