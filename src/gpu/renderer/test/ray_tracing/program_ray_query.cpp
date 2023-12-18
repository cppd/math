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

#include "program_ray_query.h"

#include "descriptors.h"

#include "code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/error.h>
#include <src/vulkan/pipeline/compute.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ns::gpu::renderer::test
{
namespace
{
class SpecializationConstants final
{
        struct Data final
        {
                std::int32_t local_size_x;
                std::int32_t local_size_y;
        };

        static constexpr std::array<VkSpecializationMapEntry, 2> ENTRIES{
                {{0, offsetof(Data, local_size_x), sizeof(Data::local_size_x)},
                 {1, offsetof(Data, local_size_y), sizeof(Data::local_size_y)}}
        };

        Data data_;

        VkSpecializationInfo info_{
                .mapEntryCount = ENTRIES.size(),
                .pMapEntries = ENTRIES.data(),
                .dataSize = sizeof(data_),
                .pData = &data_};

public:
        explicit SpecializationConstants(const std::int32_t local_size)
                : data_{.local_size_x = local_size, .local_size_y = local_size}
        {
        }

        SpecializationConstants(const SpecializationConstants&) = delete;
        SpecializationConstants& operator=(const SpecializationConstants&) = delete;

        [[nodiscard]] const VkSpecializationInfo& info() const
        {
                return info_;
        }
};
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

        const SpecializationConstants constants(local_size);

        vulkan::ComputePipelineCreateInfo compute_info;
        compute_info.device = device;
        compute_info.pipeline_layout = pipeline_layout_;
        compute_info.shader = &shader;
        compute_info.constants = &constants.info();
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
