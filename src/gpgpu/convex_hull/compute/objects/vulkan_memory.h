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

#include "com/matrix.h"
#include "graphics/glsl.h"
#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/descriptor.h"
#include "graphics/vulkan/objects.h"

#include <vector>

namespace gpgpu_vulkan
{
namespace convex_hull_compute_implementation
{
class DebugMemory
{
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;
        vulkan::DescriptorSet m_descriptor_set;

public:
        DebugMemory(const vulkan::Device& device);

        DebugMemory(const DebugMemory&) = delete;
        DebugMemory& operator=(const DebugMemory&) = delete;
        DebugMemory& operator=(DebugMemory&&) = delete;

        DebugMemory(DebugMemory&&) = default;
        ~DebugMemory() = default;

        //

        VkDescriptorSetLayout descriptor_set_layout() const noexcept;
        VkDescriptorSet descriptor_set() const noexcept;

        //

        void set_object_image(const vulkan::StorageImage& storage_image) const;
        void set_points(const vulkan::BufferWithHostVisibleMemory& buffer) const;
        void set_point_count(const vulkan::BufferWithHostVisibleMemory& buffer) const;
};

class DebugConstant
{
        struct Data
        {
                uint32_t local_size_x;
                uint32_t local_size_y;
                uint32_t local_size_z;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

public:
        DebugConstant();

        void set_local_size_x(uint32_t v);
        void set_local_size_y(uint32_t v);
        void set_local_size_z(uint32_t v);

        const std::vector<VkSpecializationMapEntry>* entries() const noexcept;
        const void* data() const noexcept;
        size_t size() const noexcept;
};

//

class PrepareMemory
{
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;
        vulkan::DescriptorSet m_descriptor_set;

public:
        PrepareMemory(const vulkan::Device& device);

        PrepareMemory(const PrepareMemory&) = delete;
        PrepareMemory& operator=(const PrepareMemory&) = delete;
        PrepareMemory& operator=(PrepareMemory&&) = delete;

        PrepareMemory(PrepareMemory&&) = default;
        ~PrepareMemory() = default;

        //

        VkDescriptorSetLayout descriptor_set_layout() const noexcept;
        VkDescriptorSet descriptor_set() const noexcept;

        //

        void set_object_image(const vulkan::StorageImage& storage_image) const;
        void set_lines(const vulkan::BufferWithHostVisibleMemory& buffer) const;
};

class PrepareConstant
{
        struct Data
        {
                int32_t line_size;
                uint32_t local_size_x;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

public:
        PrepareConstant();

        void set_line_size(int32_t v);
        void set_local_size_x(uint32_t v);

        const std::vector<VkSpecializationMapEntry>* entries() const noexcept;
        const void* data() const noexcept;
        size_t size() const noexcept;
};

//

class MergeMemory
{
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;
        vulkan::DescriptorSet m_descriptor_set;

public:
        MergeMemory(const vulkan::Device& device);

        MergeMemory(const MergeMemory&) = delete;
        MergeMemory& operator=(const MergeMemory&) = delete;
        MergeMemory& operator=(MergeMemory&&) = delete;

        MergeMemory(MergeMemory&&) = default;
        ~MergeMemory() = default;

        //

        VkDescriptorSetLayout descriptor_set_layout() const noexcept;
        VkDescriptorSet descriptor_set() const noexcept;

        //

        void set_lines(const vulkan::BufferWithHostVisibleMemory& buffer) const;
};

class MergeConstant
{
        struct Data
        {
                int32_t line_size;
                int32_t iteration_count;
                uint32_t local_size_x;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

public:
        MergeConstant();

        void set_line_size(int32_t v);
        void set_iteration_count(int32_t v);
        void set_local_size_x(uint32_t v);

        const std::vector<VkSpecializationMapEntry>* entries() const noexcept;
        const void* data() const noexcept;
        size_t size() const noexcept;
};

//

class FilterMemory
{
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;
        vulkan::DescriptorSet m_descriptor_set;

public:
        FilterMemory(const vulkan::Device& device);

        FilterMemory(const FilterMemory&) = delete;
        FilterMemory& operator=(const FilterMemory&) = delete;
        FilterMemory& operator=(FilterMemory&&) = delete;

        FilterMemory(FilterMemory&&) = default;
        ~FilterMemory() = default;

        //

        VkDescriptorSetLayout descriptor_set_layout() const noexcept;
        VkDescriptorSet descriptor_set() const noexcept;

        //

        void set_lines(const vulkan::BufferWithHostVisibleMemory& buffer) const;
        void set_points(const vulkan::BufferWithHostVisibleMemory& buffer) const;
        void set_point_count(const vulkan::BufferWithHostVisibleMemory& buffer) const;
};

class FilterConstant
{
        struct Data
        {
                int32_t line_size;
                uint32_t local_size_x;
        } m_data;

        std::vector<VkSpecializationMapEntry> m_entries;

public:
        FilterConstant();

        void set_line_size(int32_t v);
        void set_local_size_x(uint32_t v);

        const std::vector<VkSpecializationMapEntry>* entries() const noexcept;
        const void* data() const noexcept;
        size_t size() const noexcept;
};
}
}
