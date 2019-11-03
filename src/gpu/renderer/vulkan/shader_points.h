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
#include "com/vec.h"
#include "gpu/com/glsl.h"
#include "gpu/vulkan_interfaces.h"
#include "graphics/vulkan/buffers.h"
#include "graphics/vulkan/descriptor.h"
#include "graphics/vulkan/objects.h"
#include "graphics/vulkan/shader.h"

#include <vector>

namespace gpu_vulkan
{
class RendererPointsMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int MATRICES_BINDING = 0;
        static constexpr int DRAWING_BINDING = 1;
        static constexpr int OBJECTS_BINDING = 2;

        vulkan::Descriptors m_descriptors;
        std::vector<vulkan::BufferWithMemory> m_uniform_buffers;

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
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        RendererPointsMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout,
                             const std::unordered_set<uint32_t>& family_indices);

        RendererPointsMemory(const RendererPointsMemory&) = delete;
        RendererPointsMemory& operator=(const RendererPointsMemory&) = delete;
        RendererPointsMemory& operator=(RendererPointsMemory&&) = delete;

        RendererPointsMemory(RendererPointsMemory&&) = default;
        ~RendererPointsMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_matrix(const mat4& matrix) const;

        void set_default_color(const Color& color) const;
        void set_background_color(const Color& color) const;
        void set_light_a(const Color& color) const;
        void set_show_fog(bool show) const;

        void set_object_image(const vulkan::ImageWithMemory* storage_image) const;
};

struct RendererPointsVertex
{
        vec3f position;

        constexpr RendererPointsVertex(const vec3f& position_) : position(position_)
        {
        }

        static std::vector<VkVertexInputBindingDescription> binding_descriptions();

        static std::vector<VkVertexInputAttributeDescription> attribute_descriptions();
};

class RendererPointsProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::VertexShader m_vertex_shader_0d;
        vulkan::VertexShader m_vertex_shader_1d;
        vulkan::FragmentShader m_fragment_shader;
        VkPipeline m_pipeline_0d;
        VkPipeline m_pipeline_1d;

public:
        RendererPointsProgram(const vulkan::Device& device);

        RendererPointsProgram(const RendererPointsProgram&) = delete;
        RendererPointsProgram& operator=(const RendererPointsProgram&) = delete;
        RendererPointsProgram& operator=(RendererPointsProgram&&) = delete;

        RendererPointsProgram(RendererPointsProgram&&) = default;
        ~RendererPointsProgram() = default;

        void create_pipelines(RenderBuffers3D* render_buffers, unsigned x, unsigned y, unsigned width, unsigned height);
        void delete_pipelines();

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
        VkPipeline pipeline_0d() const;
        VkPipeline pipeline_1d() const;
};
}
