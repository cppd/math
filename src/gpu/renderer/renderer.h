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

#include <src/color/color.h>
#include <src/gpu/buffers.h>
#include <src/model/mesh_object.h>
#include <src/model/volume_object.h>
#include <src/numerical/matrix.h>
#include <src/numerical/region.h>
#include <src/numerical/vec.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/instance.h>
#include <src/vulkan/swapchain.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace gpu::renderer
{
struct CameraInfo final
{
        struct Volume final
        {
                double left, right, bottom, top, near, far;
        };

        Volume main_volume;
        Volume shadow_volume;
        mat4 main_view_matrix;
        mat4 shadow_view_matrix;
        vec3 light_direction;
        vec3 camera_direction;
};

struct Renderer
{
        static std::vector<vulkan::PhysicalDeviceFeatures> required_device_features();

        virtual ~Renderer() = default;

        virtual void set_light_a(const Color& light) = 0;
        virtual void set_light_d(const Color& light) = 0;
        virtual void set_light_s(const Color& light) = 0;
        virtual void set_background_color(const Color& color) = 0;
        virtual void set_default_color(const Color& color) = 0;
        virtual void set_default_specular_color(const Color& color) = 0;
        virtual void set_wireframe_color(const Color& color) = 0;
        virtual void set_clip_plane_color(const Color& color) = 0;
        virtual void set_normal_length(float length) = 0;
        virtual void set_normal_color_positive(const Color& color) = 0;
        virtual void set_normal_color_negative(const Color& color) = 0;
        virtual void set_default_ns(double default_ns) = 0;
        virtual void set_show_smooth(bool show) = 0;
        virtual void set_show_wireframe(bool show) = 0;
        virtual void set_show_shadow(bool show) = 0;
        virtual void set_show_fog(bool show) = 0;
        virtual void set_show_materials(bool show) = 0;
        virtual void set_show_normals(bool show) = 0;
        virtual void set_shadow_zoom(double zoom) = 0;
        virtual void set_camera(const CameraInfo& c) = 0;
        virtual void set_clip_plane(const std::optional<vec4>& plane) = 0;

        virtual void object_update(const mesh::MeshObject<3>& object) = 0;
        virtual void object_update(const volume::VolumeObject<3>& object) = 0;
        virtual void object_delete(ObjectId id) = 0;
        virtual void object_show(ObjectId id, bool show) = 0;
        virtual void object_delete_all() = 0;

        virtual VkSemaphore draw(
                const vulkan::Queue& graphics_queue_1,
                const vulkan::Queue& graphics_queue_2,
                unsigned image_index) const = 0;

        virtual bool empty() const = 0;

        virtual void create_buffers(
                const vulkan::Swapchain* swapchain,
                RenderBuffers3D* render_buffers,
                const vulkan::ImageWithMemory* objects,
                const Region<2, int>& viewport) = 0;
        virtual void delete_buffers() = 0;
};

std::unique_ptr<Renderer> create_renderer(
        const vulkan::VulkanInstance& instance,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue,
        bool sample_shading,
        bool sampler_anisotropy);
}
