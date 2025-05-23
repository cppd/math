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

#include "format.h"

#include <src/com/enum.h>
#include <src/com/print.h>

#include <vulkan/vulkan_core.h>

#include <string>

#define CASE(parameter) \
        case parameter: \
                return #parameter;

namespace ns::vulkan::strings
{
std::string format_to_string(const VkFormat format)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        switch (format)
        {
                CASE(VK_FORMAT_UNDEFINED)
                CASE(VK_FORMAT_R4G4_UNORM_PACK8)
                CASE(VK_FORMAT_R4G4B4A4_UNORM_PACK16)
                CASE(VK_FORMAT_B4G4R4A4_UNORM_PACK16)
                CASE(VK_FORMAT_R5G6B5_UNORM_PACK16)
                CASE(VK_FORMAT_B5G6R5_UNORM_PACK16)
                CASE(VK_FORMAT_R5G5B5A1_UNORM_PACK16)
                CASE(VK_FORMAT_B5G5R5A1_UNORM_PACK16)
                CASE(VK_FORMAT_A1R5G5B5_UNORM_PACK16)
                CASE(VK_FORMAT_R8_UNORM)
                CASE(VK_FORMAT_R8_SNORM)
                CASE(VK_FORMAT_R8_USCALED)
                CASE(VK_FORMAT_R8_SSCALED)
                CASE(VK_FORMAT_R8_UINT)
                CASE(VK_FORMAT_R8_SINT)
                CASE(VK_FORMAT_R8_SRGB)
                CASE(VK_FORMAT_R8G8_UNORM)
                CASE(VK_FORMAT_R8G8_SNORM)
                CASE(VK_FORMAT_R8G8_USCALED)
                CASE(VK_FORMAT_R8G8_SSCALED)
                CASE(VK_FORMAT_R8G8_UINT)
                CASE(VK_FORMAT_R8G8_SINT)
                CASE(VK_FORMAT_R8G8_SRGB)
                CASE(VK_FORMAT_R8G8B8_UNORM)
                CASE(VK_FORMAT_R8G8B8_SNORM)
                CASE(VK_FORMAT_R8G8B8_USCALED)
                CASE(VK_FORMAT_R8G8B8_SSCALED)
                CASE(VK_FORMAT_R8G8B8_UINT)
                CASE(VK_FORMAT_R8G8B8_SINT)
                CASE(VK_FORMAT_R8G8B8_SRGB)
                CASE(VK_FORMAT_B8G8R8_UNORM)
                CASE(VK_FORMAT_B8G8R8_SNORM)
                CASE(VK_FORMAT_B8G8R8_USCALED)
                CASE(VK_FORMAT_B8G8R8_SSCALED)
                CASE(VK_FORMAT_B8G8R8_UINT)
                CASE(VK_FORMAT_B8G8R8_SINT)
                CASE(VK_FORMAT_B8G8R8_SRGB)
                CASE(VK_FORMAT_R8G8B8A8_UNORM)
                CASE(VK_FORMAT_R8G8B8A8_SNORM)
                CASE(VK_FORMAT_R8G8B8A8_USCALED)
                CASE(VK_FORMAT_R8G8B8A8_SSCALED)
                CASE(VK_FORMAT_R8G8B8A8_UINT)
                CASE(VK_FORMAT_R8G8B8A8_SINT)
                CASE(VK_FORMAT_R8G8B8A8_SRGB)
                CASE(VK_FORMAT_B8G8R8A8_UNORM)
                CASE(VK_FORMAT_B8G8R8A8_SNORM)
                CASE(VK_FORMAT_B8G8R8A8_USCALED)
                CASE(VK_FORMAT_B8G8R8A8_SSCALED)
                CASE(VK_FORMAT_B8G8R8A8_UINT)
                CASE(VK_FORMAT_B8G8R8A8_SINT)
                CASE(VK_FORMAT_B8G8R8A8_SRGB)
                CASE(VK_FORMAT_A8B8G8R8_UNORM_PACK32)
                CASE(VK_FORMAT_A8B8G8R8_SNORM_PACK32)
                CASE(VK_FORMAT_A8B8G8R8_USCALED_PACK32)
                CASE(VK_FORMAT_A8B8G8R8_SSCALED_PACK32)
                CASE(VK_FORMAT_A8B8G8R8_UINT_PACK32)
                CASE(VK_FORMAT_A8B8G8R8_SINT_PACK32)
                CASE(VK_FORMAT_A8B8G8R8_SRGB_PACK32)
                CASE(VK_FORMAT_A2R10G10B10_UNORM_PACK32)
                CASE(VK_FORMAT_A2R10G10B10_SNORM_PACK32)
                CASE(VK_FORMAT_A2R10G10B10_USCALED_PACK32)
                CASE(VK_FORMAT_A2R10G10B10_SSCALED_PACK32)
                CASE(VK_FORMAT_A2R10G10B10_UINT_PACK32)
                CASE(VK_FORMAT_A2R10G10B10_SINT_PACK32)
                CASE(VK_FORMAT_A2B10G10R10_UNORM_PACK32)
                CASE(VK_FORMAT_A2B10G10R10_SNORM_PACK32)
                CASE(VK_FORMAT_A2B10G10R10_USCALED_PACK32)
                CASE(VK_FORMAT_A2B10G10R10_SSCALED_PACK32)
                CASE(VK_FORMAT_A2B10G10R10_UINT_PACK32)
                CASE(VK_FORMAT_A2B10G10R10_SINT_PACK32)
                CASE(VK_FORMAT_R16_UNORM)
                CASE(VK_FORMAT_R16_SNORM)
                CASE(VK_FORMAT_R16_USCALED)
                CASE(VK_FORMAT_R16_SSCALED)
                CASE(VK_FORMAT_R16_UINT)
                CASE(VK_FORMAT_R16_SINT)
                CASE(VK_FORMAT_R16_SFLOAT)
                CASE(VK_FORMAT_R16G16_UNORM)
                CASE(VK_FORMAT_R16G16_SNORM)
                CASE(VK_FORMAT_R16G16_USCALED)
                CASE(VK_FORMAT_R16G16_SSCALED)
                CASE(VK_FORMAT_R16G16_UINT)
                CASE(VK_FORMAT_R16G16_SINT)
                CASE(VK_FORMAT_R16G16_SFLOAT)
                CASE(VK_FORMAT_R16G16B16_UNORM)
                CASE(VK_FORMAT_R16G16B16_SNORM)
                CASE(VK_FORMAT_R16G16B16_USCALED)
                CASE(VK_FORMAT_R16G16B16_SSCALED)
                CASE(VK_FORMAT_R16G16B16_UINT)
                CASE(VK_FORMAT_R16G16B16_SINT)
                CASE(VK_FORMAT_R16G16B16_SFLOAT)
                CASE(VK_FORMAT_R16G16B16A16_UNORM)
                CASE(VK_FORMAT_R16G16B16A16_SNORM)
                CASE(VK_FORMAT_R16G16B16A16_USCALED)
                CASE(VK_FORMAT_R16G16B16A16_SSCALED)
                CASE(VK_FORMAT_R16G16B16A16_UINT)
                CASE(VK_FORMAT_R16G16B16A16_SINT)
                CASE(VK_FORMAT_R16G16B16A16_SFLOAT)
                CASE(VK_FORMAT_R32_UINT)
                CASE(VK_FORMAT_R32_SINT)
                CASE(VK_FORMAT_R32_SFLOAT)
                CASE(VK_FORMAT_R32G32_UINT)
                CASE(VK_FORMAT_R32G32_SINT)
                CASE(VK_FORMAT_R32G32_SFLOAT)
                CASE(VK_FORMAT_R32G32B32_UINT)
                CASE(VK_FORMAT_R32G32B32_SINT)
                CASE(VK_FORMAT_R32G32B32_SFLOAT)
                CASE(VK_FORMAT_R32G32B32A32_UINT)
                CASE(VK_FORMAT_R32G32B32A32_SINT)
                CASE(VK_FORMAT_R32G32B32A32_SFLOAT)
                CASE(VK_FORMAT_R64_UINT)
                CASE(VK_FORMAT_R64_SINT)
                CASE(VK_FORMAT_R64_SFLOAT)
                CASE(VK_FORMAT_R64G64_UINT)
                CASE(VK_FORMAT_R64G64_SINT)
                CASE(VK_FORMAT_R64G64_SFLOAT)
                CASE(VK_FORMAT_R64G64B64_UINT)
                CASE(VK_FORMAT_R64G64B64_SINT)
                CASE(VK_FORMAT_R64G64B64_SFLOAT)
                CASE(VK_FORMAT_R64G64B64A64_UINT)
                CASE(VK_FORMAT_R64G64B64A64_SINT)
                CASE(VK_FORMAT_R64G64B64A64_SFLOAT)
                CASE(VK_FORMAT_B10G11R11_UFLOAT_PACK32)
                CASE(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32)
                CASE(VK_FORMAT_D16_UNORM)
                CASE(VK_FORMAT_X8_D24_UNORM_PACK32)
                CASE(VK_FORMAT_D32_SFLOAT)
                CASE(VK_FORMAT_S8_UINT)
                CASE(VK_FORMAT_D16_UNORM_S8_UINT)
                CASE(VK_FORMAT_D24_UNORM_S8_UINT)
                CASE(VK_FORMAT_D32_SFLOAT_S8_UINT)
                CASE(VK_FORMAT_BC1_RGB_UNORM_BLOCK)
                CASE(VK_FORMAT_BC1_RGB_SRGB_BLOCK)
                CASE(VK_FORMAT_BC1_RGBA_UNORM_BLOCK)
                CASE(VK_FORMAT_BC1_RGBA_SRGB_BLOCK)
                CASE(VK_FORMAT_BC2_UNORM_BLOCK)
                CASE(VK_FORMAT_BC2_SRGB_BLOCK)
                CASE(VK_FORMAT_BC3_UNORM_BLOCK)
                CASE(VK_FORMAT_BC3_SRGB_BLOCK)
                CASE(VK_FORMAT_BC4_UNORM_BLOCK)
                CASE(VK_FORMAT_BC4_SNORM_BLOCK)
                CASE(VK_FORMAT_BC5_UNORM_BLOCK)
                CASE(VK_FORMAT_BC5_SNORM_BLOCK)
                CASE(VK_FORMAT_BC6H_UFLOAT_BLOCK)
                CASE(VK_FORMAT_BC6H_SFLOAT_BLOCK)
                CASE(VK_FORMAT_BC7_UNORM_BLOCK)
                CASE(VK_FORMAT_BC7_SRGB_BLOCK)
                CASE(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK)
                CASE(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK)
                CASE(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK)
                CASE(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK)
                CASE(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK)
                CASE(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK)
                CASE(VK_FORMAT_EAC_R11_UNORM_BLOCK)
                CASE(VK_FORMAT_EAC_R11_SNORM_BLOCK)
                CASE(VK_FORMAT_EAC_R11G11_UNORM_BLOCK)
                CASE(VK_FORMAT_EAC_R11G11_SNORM_BLOCK)
                CASE(VK_FORMAT_ASTC_4x4_UNORM_BLOCK)
                CASE(VK_FORMAT_ASTC_4x4_SRGB_BLOCK)
                CASE(VK_FORMAT_ASTC_5x4_UNORM_BLOCK)
                CASE(VK_FORMAT_ASTC_5x4_SRGB_BLOCK)
                CASE(VK_FORMAT_ASTC_5x5_UNORM_BLOCK)
                CASE(VK_FORMAT_ASTC_5x5_SRGB_BLOCK)
                CASE(VK_FORMAT_ASTC_6x5_UNORM_BLOCK)
                CASE(VK_FORMAT_ASTC_6x5_SRGB_BLOCK)
                CASE(VK_FORMAT_ASTC_6x6_UNORM_BLOCK)
                CASE(VK_FORMAT_ASTC_6x6_SRGB_BLOCK)
                CASE(VK_FORMAT_ASTC_8x5_UNORM_BLOCK)
                CASE(VK_FORMAT_ASTC_8x5_SRGB_BLOCK)
                CASE(VK_FORMAT_ASTC_8x6_UNORM_BLOCK)
                CASE(VK_FORMAT_ASTC_8x6_SRGB_BLOCK)
                CASE(VK_FORMAT_ASTC_8x8_UNORM_BLOCK)
                CASE(VK_FORMAT_ASTC_8x8_SRGB_BLOCK)
                CASE(VK_FORMAT_ASTC_10x5_UNORM_BLOCK)
                CASE(VK_FORMAT_ASTC_10x5_SRGB_BLOCK)
                CASE(VK_FORMAT_ASTC_10x6_UNORM_BLOCK)
                CASE(VK_FORMAT_ASTC_10x6_SRGB_BLOCK)
                CASE(VK_FORMAT_ASTC_10x8_UNORM_BLOCK)
                CASE(VK_FORMAT_ASTC_10x8_SRGB_BLOCK)
                CASE(VK_FORMAT_ASTC_10x10_UNORM_BLOCK)
                CASE(VK_FORMAT_ASTC_10x10_SRGB_BLOCK)
                CASE(VK_FORMAT_ASTC_12x10_UNORM_BLOCK)
                CASE(VK_FORMAT_ASTC_12x10_SRGB_BLOCK)
                CASE(VK_FORMAT_ASTC_12x12_UNORM_BLOCK)
                CASE(VK_FORMAT_ASTC_12x12_SRGB_BLOCK)
                CASE(VK_FORMAT_G8B8G8R8_422_UNORM)
                CASE(VK_FORMAT_B8G8R8G8_422_UNORM)
                CASE(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM)
                CASE(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM)
                CASE(VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM)
                CASE(VK_FORMAT_G8_B8R8_2PLANE_422_UNORM)
                CASE(VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM)
                CASE(VK_FORMAT_R10X6_UNORM_PACK16)
                CASE(VK_FORMAT_R10X6G10X6_UNORM_2PACK16)
                CASE(VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16)
                CASE(VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16)
                CASE(VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16)
                CASE(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16)
                CASE(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16)
                CASE(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16)
                CASE(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16)
                CASE(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16)
                CASE(VK_FORMAT_R12X4_UNORM_PACK16)
                CASE(VK_FORMAT_R12X4G12X4_UNORM_2PACK16)
                CASE(VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16)
                CASE(VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16)
                CASE(VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16)
                CASE(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16)
                CASE(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16)
                CASE(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16)
                CASE(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16)
                CASE(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16)
                CASE(VK_FORMAT_G16B16G16R16_422_UNORM)
                CASE(VK_FORMAT_B16G16R16G16_422_UNORM)
                CASE(VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM)
                CASE(VK_FORMAT_G16_B16R16_2PLANE_420_UNORM)
                CASE(VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM)
                CASE(VK_FORMAT_G16_B16R16_2PLANE_422_UNORM)
                CASE(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM)
                CASE(VK_FORMAT_G8_B8R8_2PLANE_444_UNORM)
                CASE(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16)
                CASE(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16)
                CASE(VK_FORMAT_G16_B16R16_2PLANE_444_UNORM)
                CASE(VK_FORMAT_A4R4G4B4_UNORM_PACK16)
                CASE(VK_FORMAT_A4B4G4R4_UNORM_PACK16)
                CASE(VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK)
                CASE(VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK)
                CASE(VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK)
                CASE(VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK)
                CASE(VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK)
                CASE(VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK)
                CASE(VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK)
                CASE(VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK)
                CASE(VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK)
                CASE(VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK)
                CASE(VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK)
                CASE(VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK)
                CASE(VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK)
                CASE(VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK)
                CASE(VK_FORMAT_A1B5G5R5_UNORM_PACK16)
                CASE(VK_FORMAT_A8_UNORM)
                CASE(VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG)
                CASE(VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG)
                CASE(VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG)
                CASE(VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG)
                CASE(VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG)
                CASE(VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG)
                CASE(VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG)
                CASE(VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG)
                CASE(VK_FORMAT_R16G16_SFIXED5_NV)
        }
#pragma GCC diagnostic pop

        return "Unknown VkFormat " + to_string(enum_to_int(format));
}
}
