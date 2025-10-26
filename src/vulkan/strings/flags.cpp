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
#include <cstdint>
#include <ios>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

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

template <std::uintmax_t TEST_FLAGS, typename Flags>
void add_flags(const std::string_view name, std::vector<std::string>* const strings, Flags* const flags)
{
        static_assert(std::is_unsigned_v<Flags>);
        static_assert(TEST_FLAGS > 0);

        constexpr Flags TEST{TEST_FLAGS};

        if ((*flags & TEST) == TEST)
        {
                strings->emplace_back(name);
                *flags &= ~TEST;
        }
}

template <typename Flags>
void add_flags_unknown(const Flags flags, std::vector<std::string>* const strings)
{
        if (!flags)
        {
                return;
        }

        std::string s;

        s += "UNKNOWN (";
        s += to_string_binary(flags, "0b");
        s += ", ";
        s += to_hex_flags(flags);
        s += ")";

        strings->push_back(s);
}
}

std::vector<std::string> sample_counts_to_strings(VkSampleCountFlags flags)
{
        std::vector<std::string> res;

        add_flags<VK_SAMPLE_COUNT_1_BIT>("1", &res, &flags);
        add_flags<VK_SAMPLE_COUNT_2_BIT>("2", &res, &flags);
        add_flags<VK_SAMPLE_COUNT_4_BIT>("4", &res, &flags);
        add_flags<VK_SAMPLE_COUNT_8_BIT>("8", &res, &flags);
        add_flags<VK_SAMPLE_COUNT_16_BIT>("16", &res, &flags);
        add_flags<VK_SAMPLE_COUNT_32_BIT>("32", &res, &flags);
        add_flags<VK_SAMPLE_COUNT_64_BIT>("64", &res, &flags);

        add_flags_unknown(flags, &res);

        return res;
}

std::vector<std::string> resolve_modes_to_strings(VkResolveModeFlags flags)
{
        std::vector<std::string> res;

        add_flags<VK_RESOLVE_MODE_SAMPLE_ZERO_BIT>("SAMPLE_ZERO", &res, &flags);
        add_flags<VK_RESOLVE_MODE_AVERAGE_BIT>("AVERAGE", &res, &flags);
        add_flags<VK_RESOLVE_MODE_MIN_BIT>("MIN", &res, &flags);
        add_flags<VK_RESOLVE_MODE_MAX_BIT>("MAX", &res, &flags);

        add_flags_unknown(flags, &res);

        return res;
}

std::vector<std::string> shader_stages_to_strings(VkShaderStageFlags flags)
{
        std::vector<std::string> res;

        add_flags<VK_SHADER_STAGE_VERTEX_BIT>("VERTEX", &res, &flags);
        add_flags<VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT>("TESSELLATION_CONTROL", &res, &flags);
        add_flags<VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT>("TESSELLATION_EVALUATION", &res, &flags);
        add_flags<VK_SHADER_STAGE_GEOMETRY_BIT>("GEOMETRY", &res, &flags);
        add_flags<VK_SHADER_STAGE_FRAGMENT_BIT>("FRAGMENT", &res, &flags);
        add_flags<VK_SHADER_STAGE_COMPUTE_BIT>("COMPUTE", &res, &flags);
        add_flags<VK_SHADER_STAGE_RAYGEN_BIT_KHR>("RAYGEN", &res, &flags);
        add_flags<VK_SHADER_STAGE_ANY_HIT_BIT_KHR>("ANY_HIT", &res, &flags);
        add_flags<VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR>("CLOSEST_HIT", &res, &flags);
        add_flags<VK_SHADER_STAGE_MISS_BIT_KHR>("MISS", &res, &flags);
        add_flags<VK_SHADER_STAGE_INTERSECTION_BIT_KHR>("INTERSECTION", &res, &flags);
        add_flags<VK_SHADER_STAGE_CALLABLE_BIT_KHR>("CALLABLE", &res, &flags);
        add_flags<VK_SHADER_STAGE_TASK_BIT_EXT>("TASK", &res, &flags);
        add_flags<VK_SHADER_STAGE_MESH_BIT_EXT>("MESH", &res, &flags);

        add_flags_unknown(flags, &res);

        return res;
}

std::vector<std::string> subgroup_features_to_strings(VkSubgroupFeatureFlags flags)
{
        std::vector<std::string> res;

        add_flags<VK_SUBGROUP_FEATURE_BASIC_BIT>("BASIC", &res, &flags);
        add_flags<VK_SUBGROUP_FEATURE_VOTE_BIT>("VOTE", &res, &flags);
        add_flags<VK_SUBGROUP_FEATURE_ARITHMETIC_BIT>("ARITHMETIC", &res, &flags);
        add_flags<VK_SUBGROUP_FEATURE_BALLOT_BIT>("BALLOT", &res, &flags);
        add_flags<VK_SUBGROUP_FEATURE_SHUFFLE_BIT>("SHUFFLE", &res, &flags);
        add_flags<VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT>("SHUFFLE_RELATIVE", &res, &flags);
        add_flags<VK_SUBGROUP_FEATURE_CLUSTERED_BIT>("CLUSTERED", &res, &flags);
        add_flags<VK_SUBGROUP_FEATURE_QUAD_BIT>("QUAD", &res, &flags);
        add_flags<VK_SUBGROUP_FEATURE_ROTATE_BIT>("ROTATE", &res, &flags);
        add_flags<VK_SUBGROUP_FEATURE_ROTATE_CLUSTERED_BIT>("ROTATE_CLUSTERED", &res, &flags);

        add_flags_unknown(flags, &res);

        return res;
}

std::vector<std::string> queues_to_strings(VkQueueFlags flags)
{
        std::vector<std::string> res;

        add_flags<VK_QUEUE_GRAPHICS_BIT>("GRAPHICS", &res, &flags);
        add_flags<VK_QUEUE_COMPUTE_BIT>("COMPUTE", &res, &flags);
        add_flags<VK_QUEUE_TRANSFER_BIT>("TRANSFER", &res, &flags);
        add_flags<VK_QUEUE_SPARSE_BINDING_BIT>("SPARSE_BINDING", &res, &flags);
        add_flags<VK_QUEUE_PROTECTED_BIT>("PROTECTED", &res, &flags);
        add_flags<VK_QUEUE_VIDEO_DECODE_BIT_KHR>("VIDEO_DECODE", &res, &flags);
        add_flags<VK_QUEUE_VIDEO_ENCODE_BIT_KHR>("VIDEO_ENCODE", &res, &flags);

        add_flags_unknown(flags, &res);

        return res;
}
}
