/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/constant.h"
#include "graphics/vulkan/descriptor.h"
#include "graphics/vulkan/objects.h"

#include <vector>

namespace gpgpu_convex_hull_compute_vulkan_implementation
{
class PrepareMemory final
{
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;

public:
        PrepareMemory(const vulkan::Device& device);

        PrepareMemory(const PrepareMemory&) = delete;
        PrepareMemory& operator=(const PrepareMemory&) = delete;
        PrepareMemory& operator=(PrepareMemory&&) = delete;

        PrepareMemory(PrepareMemory&&) = default;
        ~PrepareMemory() = default;

        //

        VkDescriptorSetLayout descriptor_set_layout() const noexcept;
        const VkDescriptorSet& descriptor_set() const noexcept;

        //

        void set_object_image(const vulkan::StorageImage& storage_image) const;
        void set_lines(const vulkan::BufferWithHostVisibleMemory& buffer) const;
};

class PrepareConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                uint32_t line_size;
                uint32_t buffer_size;
                uint32_t local_size_x;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const noexcept override;
        const void* data() const noexcept override;
        size_t size() const noexcept override;

public:
        PrepareConstant();

        void set_line_size(uint32_t v);
        void set_buffer_and_group_size(uint32_t v);
};

//

class MergeMemory final
{
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;

public:
        MergeMemory(const vulkan::Device& device);

        MergeMemory(const MergeMemory&) = delete;
        MergeMemory& operator=(const MergeMemory&) = delete;
        MergeMemory& operator=(MergeMemory&&) = delete;

        MergeMemory(MergeMemory&&) = default;
        ~MergeMemory() = default;

        //

        VkDescriptorSetLayout descriptor_set_layout() const noexcept;
        const VkDescriptorSet& descriptor_set() const noexcept;

        //

        void set_lines(const vulkan::BufferWithHostVisibleMemory& buffer) const;
};

class MergeConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                int32_t line_size;
                int32_t iteration_count;
                uint32_t local_size_x;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const noexcept override;
        const void* data() const noexcept override;
        size_t size() const noexcept override;

public:
        MergeConstant();

        void set_line_size(int32_t v);
        void set_iteration_count(int32_t v);
        void set_local_size_x(uint32_t v);
};

//

class FilterMemory final
{
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;

public:
        FilterMemory(const vulkan::Device& device);

        FilterMemory(const FilterMemory&) = delete;
        FilterMemory& operator=(const FilterMemory&) = delete;
        FilterMemory& operator=(FilterMemory&&) = delete;

        FilterMemory(FilterMemory&&) = default;
        ~FilterMemory() = default;

        //

        VkDescriptorSetLayout descriptor_set_layout() const noexcept;
        const VkDescriptorSet& descriptor_set() const noexcept;

        //

        void set_lines(const vulkan::BufferWithHostVisibleMemory& buffer) const;
        void set_points(const vulkan::BufferWithHostVisibleMemory& buffer) const;
        void set_point_count(const vulkan::BufferWithHostVisibleMemory& buffer) const;
};

class FilterConstant final : public vulkan::SpecializationConstant
{
        struct Data
        {
                int32_t line_size;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

        const std::vector<VkSpecializationMapEntry>& entries() const noexcept override;
        const void* data() const noexcept override;
        size_t size() const noexcept override;

public:
        FilterConstant();

        void set_line_size(int32_t v);
};
}
