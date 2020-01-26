/*
Copyright (C) 2017-2020 Topological Manifold

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
#include "com/vec.h"
#include "gpu/com/glsl.h"
#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/descriptor.h"
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/shader.h"
#include "numerical/matrix.h"

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

        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::BufferWithMemory> m_uniform_buffers;

        // Если размещать структуры в одном буфере, то требуется выравнивание каждой структуры
        // на VkPhysicalDeviceLimits::minUniformBufferOffsetAlignment для VkDescriptorBufferInfo::offset

        struct Matrices
        {
                struct M
                {
                        mat4f main_mvp_matrix;
                        mat4f shadow_mvp_texture_matrix;
                };
                struct C
                {
                        vec4f equation;
                        uint32_t enabled;
                };
                M matrices;
                C clip_plane;
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
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        RendererTrianglesSharedMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout,
                                      const std::unordered_set<uint32_t>& family_indices);

        RendererTrianglesSharedMemory(const RendererTrianglesSharedMemory&) = delete;
        RendererTrianglesSharedMemory& operator=(const RendererTrianglesSharedMemory&) = delete;
        RendererTrianglesSharedMemory& operator=(RendererTrianglesSharedMemory&&) = delete;

        RendererTrianglesSharedMemory(RendererTrianglesSharedMemory&&) = default;
        ~RendererTrianglesSharedMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_matrices(const mat4& main_mvp_matrix, const mat4& shadow_mvp_texture_matrix) const;
        void set_clip_plane(const vec4& equation, bool enabled) const;

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
        void set_shadow_texture(VkSampler sampler, const vulkan::DepthAttachment* shadow_texture) const;
        void set_object_image(const vulkan::ImageWithMemory* storage_image) const;
};

class RendererTrianglesMaterialMemory final
{
        static constexpr int SET_NUMBER = 1;

        static constexpr int MATERIAL_BINDING = 0;
        static constexpr int TEXTURE_KA_BINDING = 1;
        static constexpr int TEXTURE_KD_BINDING = 2;
        static constexpr int TEXTURE_KS_BINDING = 3;

        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::BufferWithMemory> m_uniform_buffers;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

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
                const vulkan::ImageWithMemory* texture_Ka;
                const vulkan::ImageWithMemory* texture_Kd;
                const vulkan::ImageWithMemory* texture_Ks;
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

        uint32_t descriptor_set_count() const;
        const VkDescriptorSet& descriptor_set(uint32_t index) const;
};

class RendererShadowMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int MATRICES_BINDING = 0;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::BufferWithMemory> m_uniform_buffers;

        struct Matrices
        {
                struct M
                {
                        mat4f mvp_matrix;
                };
                struct C
                {
                        vec4f equation;
                        uint32_t enabled;
                };
                M matrices;
                C clip_plane;
        };

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        RendererShadowMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout,
                             const std::unordered_set<uint32_t>& family_indices);

        RendererShadowMemory(const RendererShadowMemory&) = delete;
        RendererShadowMemory& operator=(const RendererShadowMemory&) = delete;
        RendererShadowMemory& operator=(RendererShadowMemory&&) = delete;

        RendererShadowMemory(RendererShadowMemory&&) = default;
        ~RendererShadowMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_matrix(const mat4& mvp_matrix) const;
        void set_clip_plane(const vec4& equation, bool enabled) const;
};

//

struct RendererTrianglesVertex
{
        vec3f position;
        vec3f normal;
        vec2f texture_coordinates;

        constexpr RendererTrianglesVertex(const vec3f& position_, const vec3f& normal_, const vec2f& texture_coordinates_)
                : position(position_), normal(normal_), texture_coordinates(texture_coordinates_)
        {
        }

        static std::vector<VkVertexInputBindingDescription> binding_descriptions();

        static std::vector<VkVertexInputAttributeDescription> triangles_attribute_descriptions();

        static std::vector<VkVertexInputAttributeDescription> shadow_attribute_descriptions();
};

//

class RendererTrianglesProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout_shared;
        vulkan::DescriptorSetLayout m_descriptor_set_layout_material;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::VertexShader m_vertex_shader;
        vulkan::GeometryShader m_geometry_shader;
        vulkan::FragmentShader m_fragment_shader;

public:
        explicit RendererTrianglesProgram(const vulkan::Device& device);

        RendererTrianglesProgram(const RendererTrianglesProgram&) = delete;
        RendererTrianglesProgram& operator=(const RendererTrianglesProgram&) = delete;
        RendererTrianglesProgram& operator=(RendererTrianglesProgram&&) = delete;

        RendererTrianglesProgram(RendererTrianglesProgram&&) = default;
        ~RendererTrianglesProgram() = default;

        vulkan::Pipeline create_pipeline(VkRenderPass render_pass, VkSampleCountFlagBits sample_count, bool sample_shading,
                                         unsigned x, unsigned y, unsigned width, unsigned height) const;

        VkDescriptorSetLayout descriptor_set_layout_shared() const;
        VkDescriptorSetLayout descriptor_set_layout_material() const;
        VkPipelineLayout pipeline_layout() const;
};

class RendererShadowProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::VertexShader m_vertex_shader;
        vulkan::FragmentShader m_fragment_shader;

public:
        explicit RendererShadowProgram(const vulkan::Device& device);

        RendererShadowProgram(const RendererShadowProgram&) = delete;
        RendererShadowProgram& operator=(const RendererShadowProgram&) = delete;
        RendererShadowProgram& operator=(RendererShadowProgram&&) = delete;

        RendererShadowProgram(RendererShadowProgram&&) = default;
        ~RendererShadowProgram() = default;

        vulkan::Pipeline create_pipeline(VkRenderPass render_pass, VkSampleCountFlagBits sample_count, unsigned x, unsigned y,
                                         unsigned width, unsigned height) const;

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
};
}
