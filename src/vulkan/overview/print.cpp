/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "print.h"

#include <src/com/print.h>

namespace ns::vulkan
{
namespace
{
template <typename Flags, typename Flag>
void add_flags(std::string* const s, Flags* const flags, const Flag& flag, const char* const name)
{
        if ((*flags & flag) == flag)
        {
                if (!s->empty())
                {
                        *s += ", ";
                }
                *s += name;
                *flags &= ~flag;
        }
}

template <typename Flags>
void add_flags_unknown(std::string* const s, const Flags& flags)
{
        if (flags != 0)
        {
                *s += s->empty() ? "" : ", ";
                *s += "UNKNOWN (";
                *s += to_string_binary(flags, "0b");
                *s += ")";
        }
}
}

std::string samples_to_string(VkSampleCountFlags flags)
{
        if (!flags)
        {
                return "NONE";
        }
        std::string s;
        add_flags(&s, &flags, VK_SAMPLE_COUNT_1_BIT, "1");
        add_flags(&s, &flags, VK_SAMPLE_COUNT_2_BIT, "2");
        add_flags(&s, &flags, VK_SAMPLE_COUNT_4_BIT, "4");
        add_flags(&s, &flags, VK_SAMPLE_COUNT_8_BIT, "8");
        add_flags(&s, &flags, VK_SAMPLE_COUNT_16_BIT, "16");
        add_flags(&s, &flags, VK_SAMPLE_COUNT_32_BIT, "32");
        add_flags(&s, &flags, VK_SAMPLE_COUNT_64_BIT, "64");
        add_flags_unknown(&s, flags);
        return s;
}

std::string resolve_modes_to_string(VkResolveModeFlags flags)
{
        if (!flags)
        {
                return "NONE";
        }
        std::string s;
        add_flags(&s, &flags, VK_RESOLVE_MODE_SAMPLE_ZERO_BIT, "SAMPLE_ZERO");
        add_flags(&s, &flags, VK_RESOLVE_MODE_AVERAGE_BIT, "AVERAGE");
        add_flags(&s, &flags, VK_RESOLVE_MODE_MIN_BIT, "MIN");
        add_flags(&s, &flags, VK_RESOLVE_MODE_MAX_BIT, "MAX");
        add_flags_unknown(&s, flags);
        return s;
}

std::string shader_stages_to_string(VkShaderStageFlags flags)
{
        if (!flags)
        {
                return "NONE";
        }
        std::string s;
        add_flags(&s, &flags, VK_SHADER_STAGE_VERTEX_BIT, "VERTEX");
        add_flags(&s, &flags, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, "TESSELLATION_CONTROL");
        add_flags(&s, &flags, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, "TESSELLATION_EVALUATION");
        add_flags(&s, &flags, VK_SHADER_STAGE_GEOMETRY_BIT, "GEOMETRY");
        add_flags(&s, &flags, VK_SHADER_STAGE_FRAGMENT_BIT, "FRAGMENT");
        add_flags(&s, &flags, VK_SHADER_STAGE_COMPUTE_BIT, "COMPUTE");
        add_flags_unknown(&s, flags);
        return s;
}

std::string subgroup_features_to_string(VkSubgroupFeatureFlags flags)
{
        if (!flags)
        {
                return "NONE";
        }
        std::string s;
        add_flags(&s, &flags, VK_SUBGROUP_FEATURE_BASIC_BIT, "BASIC");
        add_flags(&s, &flags, VK_SUBGROUP_FEATURE_VOTE_BIT, "VOTE");
        add_flags(&s, &flags, VK_SUBGROUP_FEATURE_ARITHMETIC_BIT, "ARITHMETIC");
        add_flags(&s, &flags, VK_SUBGROUP_FEATURE_BALLOT_BIT, "BALLOT");
        add_flags(&s, &flags, VK_SUBGROUP_FEATURE_SHUFFLE_BIT, "SHUFFLE");
        add_flags(&s, &flags, VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT, "SHUFFLE_RELATIVE");
        add_flags(&s, &flags, VK_SUBGROUP_FEATURE_CLUSTERED_BIT, "CLUSTERED");
        add_flags(&s, &flags, VK_SUBGROUP_FEATURE_QUAD_BIT, "QUAD");
        add_flags_unknown(&s, flags);
        return s;
}

std::string queues_to_string(VkQueueFlags flags)
{
        if (!flags)
        {
                return "NONE";
        }
        std::string s;
        add_flags(&s, &flags, VK_QUEUE_GRAPHICS_BIT, "GRAPHICS");
        add_flags(&s, &flags, VK_QUEUE_COMPUTE_BIT, "COMPUTE");
        add_flags(&s, &flags, VK_QUEUE_TRANSFER_BIT, "TRANSFER");
        add_flags(&s, &flags, VK_QUEUE_SPARSE_BINDING_BIT, "SPARSE_BINDING");
        add_flags(&s, &flags, VK_QUEUE_PROTECTED_BIT, "PROTECTED");
        add_flags_unknown(&s, flags);
        return s;
}

std::string shader_float_controls_independence_to_string(const VkShaderFloatControlsIndependence v)
{
        if (v == VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_32_BIT_ONLY)
        {
                return "32_BIT_ONLY";
        }

        if (v == VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL)
        {
                return "ALL";
        }
        if (v == VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_NONE)
        {
                return "NONE";
        }
        return "UNKNOWN";
}

std::string point_clipping_behavior_to_string(const VkPointClippingBehavior v)
{
        if (v == VK_POINT_CLIPPING_BEHAVIOR_ALL_CLIP_PLANES)
        {
                return "ALL_CLIP_PLANES";
        }
        if (v == VK_POINT_CLIPPING_BEHAVIOR_USER_CLIP_PLANES_ONLY)
        {
                return "USER_CLIP_PLANES_ONLY";
        }
        return "UNKNOWN";
}
}
