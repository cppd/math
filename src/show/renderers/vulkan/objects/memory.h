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

#include "com/color/color.h"
#include "com/matrix.h"
#include "com/matrix_alg.h"
#include "com/vec.h"
#include "graphics/glsl.h"
#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/descriptor.h"
#include "graphics/vulkan/objects.h"

#include <unordered_set>
#include <vector>

namespace gpu_vulkan
{
class RendererTrianglesSharedMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int MATRICES_BINDING = 0;
        static constexpr int LIGHTING_BINDING = 1;
        static constexpr int DRAWING_BINDING = 2;
        static constexpr int SHADOW_BINDING = 3;
        static constexpr int OBJECTS_BINDING = 4;

        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::BufferWithHostVisibleMemory> m_uniform_buffers;

        // Если размещать структуры в одном буфере, то требуется выравнивание каждой структуры
        // на VkPhysicalDeviceLimits::minUniformBufferOffsetAlignment для VkDescriptorBufferInfo::offset

        struct Matrices
        {
                Matrix<4, 4, float> matrix;
                Matrix<4, 4, float> shadow_matrix;
        };

        struct Lighting
        {
                alignas(GLSL_VEC3_ALIGN) vec3f direction_to_light;
                alignas(GLSL_VEC3_ALIGN) vec3f direction_to_camera;
                uint32_t show_smooth;
        };

        struct Drawing
        {
                alignas(GLSL_VEC3_ALIGN) vec3f default_color;
                alignas(GLSL_VEC3_ALIGN) vec3f wireframe_color;
                float default_ns;
                alignas(GLSL_VEC3_ALIGN) vec3f light_a;
                alignas(GLSL_VEC3_ALIGN) vec3f light_d;
                alignas(GLSL_VEC3_ALIGN) vec3f light_s;
                uint32_t show_materials;
                uint32_t show_wireframe;
                uint32_t show_shadow;
        };

        size_t m_matrices_buffer_index;
        size_t m_lighting_buffer_index;
        size_t m_drawing_buffer_index;

        template <typename T>
        void copy_to_matrices_buffer(VkDeviceSize offset, const T& data) const;
        template <typename T>
        void copy_to_lighting_buffer(VkDeviceSize offset, const T& data) const;
        template <typename T>
        void copy_to_drawing_buffer(VkDeviceSize offset, const T& data) const;

public:
        RendererTrianglesSharedMemory(const vulkan::Device& device, const std::unordered_set<uint32_t>& family_indices);

        RendererTrianglesSharedMemory(const RendererTrianglesSharedMemory&) = delete;
        RendererTrianglesSharedMemory& operator=(const RendererTrianglesSharedMemory&) = delete;
        RendererTrianglesSharedMemory& operator=(RendererTrianglesSharedMemory&&) = delete;

        RendererTrianglesSharedMemory(RendererTrianglesSharedMemory&&) = default;
        ~RendererTrianglesSharedMemory() = default;

        //

        static unsigned set_number();
        VkDescriptorSetLayout descriptor_set_layout() const;
        const VkDescriptorSet& descriptor_set() const;

        //

        void set_matrices(const mat4& matrix, const mat4& shadow_matrix) const;

        void set_default_color(const Color& color) const;
        void set_wireframe_color(const Color& color) const;
        void set_default_ns(float default_ns) const;
        void set_light_a(const Color& color) const;
        void set_light_d(const Color& color) const;
        void set_light_s(const Color& color) const;
        void set_show_materials(bool show) const;
        void set_direction_to_light(const vec3f& direction) const;
        void set_direction_to_camera(const vec3f& direction) const;
        void set_show_smooth(bool show) const;
        void set_show_wireframe(bool show) const;
        void set_show_shadow(bool show) const;
        void set_shadow_texture(VkSampler sampler, const vulkan::DepthAttachmentTexture* shadow_texture) const;
        void set_object_image(const vulkan::StorageImage* storage_image) const;
};

class RendererTrianglesMaterialMemory final
{
        static constexpr int SET_NUMBER = 1;

        static constexpr int MATERIAL_BINDING = 0;
        static constexpr int TEXTURE_KA_BINDING = 1;
        static constexpr int TEXTURE_KD_BINDING = 2;
        static constexpr int TEXTURE_KS_BINDING = 3;

        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::BufferWithHostVisibleMemory> m_uniform_buffers;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        struct Material
        {
                alignas(GLSL_VEC3_ALIGN) vec3f Ka;
                alignas(GLSL_VEC3_ALIGN) vec3f Kd;
                alignas(GLSL_VEC3_ALIGN) vec3f Ks;
                float Ns;
                uint32_t use_texture_Ka;
                uint32_t use_texture_Kd;
                uint32_t use_texture_Ks;
                uint32_t use_material;
        };

        struct MaterialAndTexture
        {
                Material material;
                const vulkan::ColorTexture* texture_Ka;
                const vulkan::ColorTexture* texture_Kd;
                const vulkan::ColorTexture* texture_Ks;
        };

        //

        RendererTrianglesMaterialMemory(const vulkan::Device& device, const std::unordered_set<uint32_t>& family_indices,
                                        VkSampler sampler, VkDescriptorSetLayout descriptor_set_layout,
                                        const std::vector<MaterialAndTexture>& materials);

        RendererTrianglesMaterialMemory(const RendererTrianglesMaterialMemory&) = delete;
        RendererTrianglesMaterialMemory& operator=(const RendererTrianglesMaterialMemory&) = delete;
        RendererTrianglesMaterialMemory& operator=(RendererTrianglesMaterialMemory&&) = delete;

        RendererTrianglesMaterialMemory(RendererTrianglesMaterialMemory&&) = default;
        ~RendererTrianglesMaterialMemory() = default;

        //

        static unsigned set_number();
        uint32_t descriptor_set_count() const;
        const VkDescriptorSet& descriptor_set(uint32_t index) const;
};

class RendererShadowMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int MATRICES_BINDING = 0;

        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::BufferWithHostVisibleMemory> m_uniform_buffers;

        struct Matrices
        {
                Matrix<4, 4, float> matrix;
        };

public:
        RendererShadowMemory(const vulkan::Device& device, const std::unordered_set<uint32_t>& family_indices);

        RendererShadowMemory(const RendererShadowMemory&) = delete;
        RendererShadowMemory& operator=(const RendererShadowMemory&) = delete;
        RendererShadowMemory& operator=(RendererShadowMemory&&) = delete;

        RendererShadowMemory(RendererShadowMemory&&) = default;
        ~RendererShadowMemory() = default;

        //

        static unsigned set_number();
        VkDescriptorSetLayout descriptor_set_layout() const;
        const VkDescriptorSet& descriptor_set() const;

        //

        void set_matrix(const mat4& matrix) const;
};

class RendererPointsMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int MATRICES_BINDING = 0;
        static constexpr int DRAWING_BINDING = 1;
        static constexpr int OBJECTS_BINDING = 2;

        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::BufferWithHostVisibleMemory> m_uniform_buffers;

        struct Matrices
        {
                Matrix<4, 4, float> matrix;
        };

        struct Drawing
        {
                alignas(GLSL_VEC3_ALIGN) vec3f default_color;
                alignas(GLSL_VEC3_ALIGN) vec3f background_color;
                alignas(GLSL_VEC3_ALIGN) vec3f light_a;
                uint32_t show_fog;
        };

        size_t m_matrices_buffer_index;
        size_t m_drawing_buffer_index;

        template <typename T>
        void copy_to_matrices_buffer(VkDeviceSize offset, const T& data) const;
        template <typename T>
        void copy_to_drawing_buffer(VkDeviceSize offset, const T& data) const;

public:
        RendererPointsMemory(const vulkan::Device& device, const std::unordered_set<uint32_t>& family_indices);

        RendererPointsMemory(const RendererPointsMemory&) = delete;
        RendererPointsMemory& operator=(const RendererPointsMemory&) = delete;
        RendererPointsMemory& operator=(RendererPointsMemory&&) = delete;

        RendererPointsMemory(RendererPointsMemory&&) = default;
        ~RendererPointsMemory() = default;

        //

        static unsigned set_number();
        VkDescriptorSetLayout descriptor_set_layout() const;
        const VkDescriptorSet& descriptor_set() const;

        //

        void set_matrix(const mat4& matrix) const;

        void set_default_color(const Color& color) const;
        void set_background_color(const Color& color) const;
        void set_light_a(const Color& color) const;
        void set_show_fog(bool show) const;

        void set_object_image(const vulkan::StorageImage* storage_image) const;
};
}
