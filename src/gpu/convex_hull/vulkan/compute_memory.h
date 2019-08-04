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

namespace gpu_vulkan
{
class ConvexHullPrepareMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int LINES_BINDING = 0;
        static constexpr int OBJECTS_BINDING = 1;

        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;

public:
        ConvexHullPrepareMemory(const vulkan::Device& device);

        ConvexHullPrepareMemory(const ConvexHullPrepareMemory&) = delete;
        ConvexHullPrepareMemory& operator=(const ConvexHullPrepareMemory&) = delete;
        ConvexHullPrepareMemory& operator=(ConvexHullPrepareMemory&&) = delete;

        ConvexHullPrepareMemory(ConvexHullPrepareMemory&&) = default;
        ~ConvexHullPrepareMemory() = default;

        //

        static unsigned set_number() noexcept;
        VkDescriptorSetLayout descriptor_set_layout() const noexcept;
        const VkDescriptorSet& descriptor_set() const noexcept;

        //

        void set_object_image(const vulkan::StorageImage& storage_image) const;
        void set_lines(const vulkan::BufferWithMemory& buffer) const;
};

class ConvexHullPrepareConstant final : public vulkan::SpecializationConstant
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
        ConvexHullPrepareConstant();

        void set_line_size(uint32_t v);
        void set_buffer_and_group_size(uint32_t v);
};

//

class ConvexHullMergeMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int LINES_BINDING = 0;

        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;

public:
        ConvexHullMergeMemory(const vulkan::Device& device);

        ConvexHullMergeMemory(const ConvexHullMergeMemory&) = delete;
        ConvexHullMergeMemory& operator=(const ConvexHullMergeMemory&) = delete;
        ConvexHullMergeMemory& operator=(ConvexHullMergeMemory&&) = delete;

        ConvexHullMergeMemory(ConvexHullMergeMemory&&) = default;
        ~ConvexHullMergeMemory() = default;

        //

        static unsigned set_number() noexcept;
        VkDescriptorSetLayout descriptor_set_layout() const noexcept;
        const VkDescriptorSet& descriptor_set() const noexcept;

        //

        void set_lines(const vulkan::BufferWithMemory& buffer) const;
};

class ConvexHullMergeConstant final : public vulkan::SpecializationConstant
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
        ConvexHullMergeConstant();

        void set_line_size(int32_t v);
        void set_iteration_count(int32_t v);
        void set_local_size_x(uint32_t v);
};

//

class ConvexHullFilterMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int LINES_BINDING = 0;
        static constexpr int POINTS_BINDING = 1;
        static constexpr int POINT_COUNT_BINDING = 2;

        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;

public:
        ConvexHullFilterMemory(const vulkan::Device& device);

        ConvexHullFilterMemory(const ConvexHullFilterMemory&) = delete;
        ConvexHullFilterMemory& operator=(const ConvexHullFilterMemory&) = delete;
        ConvexHullFilterMemory& operator=(ConvexHullFilterMemory&&) = delete;

        ConvexHullFilterMemory(ConvexHullFilterMemory&&) = default;
        ~ConvexHullFilterMemory() = default;

        //

        static unsigned set_number() noexcept;
        VkDescriptorSetLayout descriptor_set_layout() const noexcept;
        const VkDescriptorSet& descriptor_set() const noexcept;

        //

        void set_lines(const vulkan::BufferWithMemory& buffer) const;
        void set_points(const vulkan::BufferWithMemory& buffer) const;
        void set_point_count(const vulkan::BufferWithMemory& buffer) const;
};

class ConvexHullFilterConstant final : public vulkan::SpecializationConstant
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
        ConvexHullFilterConstant();

        void set_line_size(int32_t v);
};
}
