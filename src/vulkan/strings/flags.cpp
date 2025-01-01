/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "flags.h"

#include <src/com/print.h>

#include <vulkan/vulkan_core.h>

#include <bit>
#include <ios>
#include <sstream>
#include <string>
#include <string_view>

namespace ns::vulkan::strings
{
namespace
{
template <typename Flags>
std::string to_hex_flags(const Flags flags)
{
        if (!flags)
        {
                return {};
        }

        std::ostringstream oss;
        oss << std::hex;
        Flags f = 1 << (std::bit_width(flags) - 1);
        bool first = true;
        while (f)
        {
                if (flags & f)
                {
                        if (!first)
                        {
                                oss << '|';
                        }
                        else
                        {
                                first = false;
                        }
                        oss << "0x" << f;
                }
                f >>= 1;
        }
        return oss.str();
}

template <typename Flags, typename Flag>
void add_flags(
        std::string* const s,
        const Flags flags,
        Flags* const flags_clear,
        const Flag test_flags,
        const std::string_view name)
{
        if ((flags & test_flags) == test_flags)
        {
                if (!s->empty())
                {
                        *s += ", ";
                }

                *s += name;
                *flags_clear &= ~test_flags;
        }
}

template <typename Flags>
void add_flags_unknown(std::string* const s, const Flags flags)
{
        if (!flags)
        {
                return;
        }

        if (!s->empty())
        {
                *s += ", ";
        }

        *s += "UNKNOWN (";
        *s += to_string_binary(flags, "0b");
        *s += ", ";
        *s += to_hex_flags(flags);
        *s += ")";
}
}

std::string sample_counts_to_string(const VkSampleCountFlags flags)
{
        if (!flags)
        {
                return "NONE";
        }

        std::string s;
        auto flags_clear = flags;

        add_flags(&s, flags, &flags_clear, VK_SAMPLE_COUNT_1_BIT, "1");
        add_flags(&s, flags, &flags_clear, VK_SAMPLE_COUNT_2_BIT, "2");
        add_flags(&s, flags, &flags_clear, VK_SAMPLE_COUNT_4_BIT, "4");
        add_flags(&s, flags, &flags_clear, VK_SAMPLE_COUNT_8_BIT, "8");
        add_flags(&s, flags, &flags_clear, VK_SAMPLE_COUNT_16_BIT, "16");
        add_flags(&s, flags, &flags_clear, VK_SAMPLE_COUNT_32_BIT, "32");
        add_flags(&s, flags, &flags_clear, VK_SAMPLE_COUNT_64_BIT, "64");

        add_flags_unknown(&s, flags_clear);

        return s;
}

std::string resolve_modes_to_string(const VkResolveModeFlags flags)
{
        if (!flags)
        {
                return "NONE";
        }

        std::string s;
        auto flags_clear = flags;

        add_flags(&s, flags, &flags_clear, VK_RESOLVE_MODE_SAMPLE_ZERO_BIT, "SAMPLE_ZERO");
        add_flags(&s, flags, &flags_clear, VK_RESOLVE_MODE_AVERAGE_BIT, "AVERAGE");
        add_flags(&s, flags, &flags_clear, VK_RESOLVE_MODE_MIN_BIT, "MIN");
        add_flags(&s, flags, &flags_clear, VK_RESOLVE_MODE_MAX_BIT, "MAX");

        add_flags_unknown(&s, flags_clear);

        return s;
}

std::string shader_stages_to_string(const VkShaderStageFlags flags)
{
        if (!flags)
        {
                return "NONE";
        }

        std::string s;
        auto flags_clear = flags;

        add_flags(&s, flags, &flags_clear, VK_SHADER_STAGE_VERTEX_BIT, "VERTEX");
        add_flags(&s, flags, &flags_clear, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, "TESSELLATION_CONTROL");
        add_flags(&s, flags, &flags_clear, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "TESSELLATION_EVALUATION");
        add_flags(&s, flags, &flags_clear, VK_SHADER_STAGE_GEOMETRY_BIT, "GEOMETRY");
        add_flags(&s, flags, &flags_clear, VK_SHADER_STAGE_FRAGMENT_BIT, "FRAGMENT");
        add_flags(&s, flags, &flags_clear, VK_SHADER_STAGE_COMPUTE_BIT, "COMPUTE");
        add_flags(&s, flags, &flags_clear, VK_SHADER_STAGE_RAYGEN_BIT_KHR, "RAYGEN");
        add_flags(&s, flags, &flags_clear, VK_SHADER_STAGE_ANY_HIT_BIT_KHR, "ANY_HIT");
        add_flags(&s, flags, &flags_clear, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, "CLOSEST_HIT");
        add_flags(&s, flags, &flags_clear, VK_SHADER_STAGE_MISS_BIT_KHR, "MISS");
        add_flags(&s, flags, &flags_clear, VK_SHADER_STAGE_INTERSECTION_BIT_KHR, "INTERSECTION");
        add_flags(&s, flags, &flags_clear, VK_SHADER_STAGE_CALLABLE_BIT_KHR, "CALLABLE");
        add_flags(&s, flags, &flags_clear, VK_SHADER_STAGE_TASK_BIT_EXT, "TASK");
        add_flags(&s, flags, &flags_clear, VK_SHADER_STAGE_MESH_BIT_EXT, "MESH");

        add_flags_unknown(&s, flags_clear);

        return s;
}

std::string subgroup_features_to_string(const VkSubgroupFeatureFlags flags)
{
        if (!flags)
        {
                return "NONE";
        }

        std::string s;
        auto flags_clear = flags;

        add_flags(&s, flags, &flags_clear, VK_SUBGROUP_FEATURE_BASIC_BIT, "BASIC");
        add_flags(&s, flags, &flags_clear, VK_SUBGROUP_FEATURE_VOTE_BIT, "VOTE");
        add_flags(&s, flags, &flags_clear, VK_SUBGROUP_FEATURE_ARITHMETIC_BIT, "ARITHMETIC");
        add_flags(&s, flags, &flags_clear, VK_SUBGROUP_FEATURE_BALLOT_BIT, "BALLOT");
        add_flags(&s, flags, &flags_clear, VK_SUBGROUP_FEATURE_SHUFFLE_BIT, "SHUFFLE");
        add_flags(&s, flags, &flags_clear, VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT, "SHUFFLE_RELATIVE");
        add_flags(&s, flags, &flags_clear, VK_SUBGROUP_FEATURE_CLUSTERED_BIT, "CLUSTERED");
        add_flags(&s, flags, &flags_clear, VK_SUBGROUP_FEATURE_QUAD_BIT, "QUAD");

        add_flags_unknown(&s, flags_clear);

        return s;
}

std::string queues_to_string(const VkQueueFlags flags)
{
        if (!flags)
        {
                return "NONE";
        }

        std::string s;
        auto flags_clear = flags;

        add_flags(&s, flags, &flags_clear, VK_QUEUE_GRAPHICS_BIT, "GRAPHICS");
        add_flags(&s, flags, &flags_clear, VK_QUEUE_COMPUTE_BIT, "COMPUTE");
        add_flags(&s, flags, &flags_clear, VK_QUEUE_TRANSFER_BIT, "TRANSFER");
        add_flags(&s, flags, &flags_clear, VK_QUEUE_SPARSE_BINDING_BIT, "SPARSE_BINDING");
        add_flags(&s, flags, &flags_clear, VK_QUEUE_PROTECTED_BIT, "PROTECTED");

        add_flags_unknown(&s, flags_clear);

        return s;
}
}
