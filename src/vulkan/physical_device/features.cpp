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

#include "features.h"

#include "feature_properties.h"
#include "info.h"

#include <src/com/error.h>

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstring>
#include <exception>
#include <string>
#include <utility>
#include <vector>

namespace ns::vulkan::physical_device
{
namespace
{
constexpr std::size_t SIZE = sizeof(VkBool32);
constexpr VkBool32 TRUE = VK_TRUE;

class FeatureIsNotSupported final : public std::exception
{
        std::string text_;

public:
        explicit FeatureIsNotSupported(std::string&& name) noexcept
                : text_(std::move(name))
        {
        }

        [[nodiscard]] const char* what() const noexcept override
        {
                return text_.c_str();
        }
};

template <typename T>
[[noreturn]] void feature_is_not_supported_error(T&& name)
{
        throw FeatureIsNotSupported(std::forward<T>(name));
}

template <typename Features>
void add_features(Features* const dst, const Features& src)
{
        static constexpr std::size_t COUNT = FeatureProperties<Features>::COUNT;
        static constexpr std::size_t OFFSET = FeatureProperties<Features>::OFFSET;

        std::byte* dst_ptr = reinterpret_cast<std::byte*>(dst) + OFFSET;
        const std::byte* src_ptr = reinterpret_cast<const std::byte*>(&src) + OFFSET;

        for (std::size_t i = 0; i < COUNT; ++i, dst_ptr += SIZE, src_ptr += SIZE)
        {
                VkBool32 feature;
                std::memcpy(&feature, src_ptr, SIZE);
                if (!feature)
                {
                        continue;
                }
                std::memcpy(dst_ptr, &TRUE, SIZE);
        }
}

template <bool REQUIRED, typename Features>
void set_features(const Features& features, const Features& supported, Features* const result)
{
        static constexpr std::size_t COUNT = FeatureProperties<Features>::COUNT;
        static constexpr std::size_t OFFSET = FeatureProperties<Features>::OFFSET;

        const std::byte* ptr = reinterpret_cast<const std::byte*>(&features) + OFFSET;
        const std::byte* supported_ptr = reinterpret_cast<const std::byte*>(&supported) + OFFSET;
        std::byte* result_ptr = reinterpret_cast<std::byte*>(result) + OFFSET;

        for (std::size_t i = 0; i < COUNT; ++i, ptr += SIZE, supported_ptr += SIZE, result_ptr += SIZE)
        {
                VkBool32 feature;
                std::memcpy(&feature, ptr, SIZE);
                if (!feature)
                {
                        continue;
                }

                VkBool32 supported_feature;
                std::memcpy(&supported_feature, supported_ptr, SIZE);
                if (supported_feature)
                {
                        std::memcpy(result_ptr, &TRUE, SIZE);
                }
                else if (REQUIRED)
                {
                        feature_is_not_supported_error(FeatureProperties<Features>::name(i));
                }
        }
}

template <typename Features>
void check_features(const Features& required, const Features& supported)
{
        static constexpr std::size_t COUNT = FeatureProperties<Features>::COUNT;
        static constexpr std::size_t OFFSET = FeatureProperties<Features>::OFFSET;

        const std::byte* required_ptr = reinterpret_cast<const std::byte*>(&required) + OFFSET;
        const std::byte* supported_ptr = reinterpret_cast<const std::byte*>(&supported) + OFFSET;

        for (std::size_t i = 0; i < COUNT; ++i, required_ptr += SIZE, supported_ptr += SIZE)
        {
                VkBool32 required_feature;
                std::memcpy(&required_feature, required_ptr, SIZE);
                if (!required_feature)
                {
                        continue;
                }

                VkBool32 supported_feature;
                std::memcpy(&supported_feature, supported_ptr, SIZE);
                if (!supported_feature)
                {
                        feature_is_not_supported_error(FeatureProperties<Features>::name(i));
                }
        }
}

template <typename Features>
void features_to_strings(const Features& features, const bool enabled, std::vector<std::string>* const strings)
{
        static constexpr std::size_t COUNT = FeatureProperties<Features>::COUNT;
        static constexpr std::size_t OFFSET = FeatureProperties<Features>::OFFSET;

        const std::byte* ptr = reinterpret_cast<const std::byte*>(&features) + OFFSET;

        for (std::size_t i = 0; i < COUNT; ++i, ptr += SIZE)
        {
                VkBool32 feature;
                std::memcpy(&feature, ptr, SIZE);
                if (static_cast<bool>(feature) == enabled)
                {
                        strings->push_back(FeatureProperties<Features>::name(i));
                }
        }
}

template <bool REQUIRED>
void set_features(const Features& features, const Features& supported, Features* const result)
{
        set_features<REQUIRED>(features.features_10, supported.features_10, &result->features_10);

        set_features<REQUIRED>(features.features_11, supported.features_11, &result->features_11);

        set_features<REQUIRED>(features.features_12, supported.features_12, &result->features_12);

        set_features<REQUIRED>(features.features_13, supported.features_13, &result->features_13);

        set_features<REQUIRED>(
                features.acceleration_structure, supported.acceleration_structure, &result->acceleration_structure);

        set_features<REQUIRED>(features.ray_query, supported.ray_query, &result->ray_query);

        set_features<REQUIRED>(
                features.ray_tracing_pipeline, supported.ray_tracing_pipeline, &result->ray_tracing_pipeline);
}
}

void add_features(Features* const dst, const Features& src)
{
        add_features(&dst->features_10, src.features_10);
        add_features(&dst->features_11, src.features_11);
        add_features(&dst->features_12, src.features_12);
        add_features(&dst->features_13, src.features_13);
        add_features(&dst->acceleration_structure, src.acceleration_structure);
        add_features(&dst->ray_query, src.ray_query);
        add_features(&dst->ray_tracing_pipeline, src.ray_tracing_pipeline);
}

Features make_features(const Features& required, const Features& optional, const Features& supported)
{
        Features res = {};

        try
        {
                set_features<true>(required, supported, &res);
        }
        catch (const FeatureIsNotSupported& e)
        {
                error(std::string("Required physical device feature ") + e.what() + " is not supported");
        }

        try
        {
                set_features<false>(optional, supported, &res);
        }
        catch (const FeatureIsNotSupported&)
        {
                error("Exception when setting optional device features");
        }

        return res;
}

bool check_features(const Features& required, const Features& supported)
{
        try
        {
                check_features(required.features_10, supported.features_10);
                check_features(required.features_11, supported.features_11);
                check_features(required.features_12, supported.features_12);
                check_features(required.features_13, supported.features_13);
                check_features(required.acceleration_structure, supported.acceleration_structure);
                check_features(required.ray_query, supported.ray_query);
                check_features(required.ray_tracing_pipeline, supported.ray_tracing_pipeline);
        }
        catch (const FeatureIsNotSupported&)
        {
                return false;
        }
        return true;
}

std::vector<std::string> features_to_strings(const Features& features, const bool enabled)
{
        std::vector<std::string> res;

        features_to_strings(features.features_10, enabled, &res);
        features_to_strings(features.features_11, enabled, &res);
        features_to_strings(features.features_12, enabled, &res);
        features_to_strings(features.features_13, enabled, &res);
        features_to_strings(features.acceleration_structure, enabled, &res);
        features_to_strings(features.ray_query, enabled, &res);
        features_to_strings(features.ray_tracing_pipeline, enabled, &res);

        return res;
}

template <typename Features>
bool any_feature_enabled(const Features& features)
{
        static constexpr std::size_t COUNT = FeatureProperties<Features>::COUNT;
        static constexpr std::size_t OFFSET = FeatureProperties<Features>::OFFSET;

        const std::byte* ptr = reinterpret_cast<const std::byte*>(&features) + OFFSET;
        const std::byte* const end = ptr + COUNT * SIZE;

        for (; ptr != end; ptr += SIZE)
        {
                VkBool32 feature;
                std::memcpy(&feature, ptr, SIZE);
                if (feature)
                {
                        return true;
                }
        }

        return false;
}

template bool any_feature_enabled(const VkPhysicalDeviceFeatures&);
template bool any_feature_enabled(const VkPhysicalDeviceVulkan11Features&);
template bool any_feature_enabled(const VkPhysicalDeviceVulkan12Features&);
template bool any_feature_enabled(const VkPhysicalDeviceVulkan13Features&);
template bool any_feature_enabled(const VkPhysicalDeviceAccelerationStructureFeaturesKHR&);
template bool any_feature_enabled(const VkPhysicalDeviceRayQueryFeaturesKHR&);
template bool any_feature_enabled(const VkPhysicalDeviceRayTracingPipelineFeaturesKHR&);
}
