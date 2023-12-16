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

#include "objects.h"

#include <cstdint>
#include <unordered_map>
#include <variant>
#include <vector>

namespace ns::vulkan
{
handle::DescriptorSetLayout create_descriptor_set_layout(
        VkDevice device,
        const std::vector<VkDescriptorSetLayoutBinding>& bindings);

struct DescriptorSetLayoutAndBindings final
{
        VkDescriptorSetLayout descriptor_set_layout;
        std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings;

        template <typename DescriptorSetLayout, typename DescriptorSetLayoutBindings>
        DescriptorSetLayoutAndBindings(
                DescriptorSetLayout&& descriptor_set_layout,
                DescriptorSetLayoutBindings&& descriptor_set_layout_bindings)
                : descriptor_set_layout(std::forward<DescriptorSetLayout>(descriptor_set_layout)),
                  descriptor_set_layout_bindings(
                          std::forward<DescriptorSetLayoutBindings>(descriptor_set_layout_bindings))
        {
        }
};

class Descriptors final
{
        VkDevice device_;
        VkDescriptorSetLayout descriptor_set_layout_;
        handle::DescriptorPool descriptor_pool_;
        std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings_;
        handle::DescriptorSets descriptor_sets_;

        // VkDescriptorSetLayoutBinding::binding -> descriptor_set_layout_bindings_ index
        std::unordered_map<std::uint32_t, std::uint32_t> binding_map_;

        // VkDescriptorSetLayoutBinding::binding -> const VkDescriptorSetLayoutBinding&
        [[nodiscard]] const VkDescriptorSetLayoutBinding& layout_binding(std::uint32_t binding) const;

public:
        Descriptors(
                VkDevice device,
                std::uint32_t max_sets,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<VkDescriptorSetLayoutBinding>& bindings);

        Descriptors(const Descriptors&) = delete;
        Descriptors& operator=(const Descriptors&) = delete;
        Descriptors& operator=(Descriptors&&) = delete;

        Descriptors(Descriptors&&) = default;
        ~Descriptors() = default;

        //

        [[nodiscard]] VkDescriptorSetLayout descriptor_set_layout() const noexcept;

        [[nodiscard]] std::uint32_t descriptor_set_count() const;
        [[nodiscard]] const VkDescriptorSet& descriptor_set(std::uint32_t index) const;

        using Info = std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo, VkAccelerationStructureKHR>;

        struct DescriptorInfo final
        {
                std::uint32_t index;
                std::uint32_t binding;
                Info info;

                DescriptorInfo(const std::uint32_t index, const std::uint32_t binding, const Info& info)
                        : index(index),
                          binding(binding),
                          info(info)
                {
                }
        };

        void update_descriptor_set(std::uint32_t index, std::uint32_t binding, const Info& info) const;

        void update_descriptor_set(const std::vector<DescriptorInfo>& infos) const;
};
}
