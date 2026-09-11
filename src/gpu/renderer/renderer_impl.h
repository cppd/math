/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "acceleration_structure.h"
#include "event.h"
#include "renderer.h"
#include "renderer_draw.h"
#include "renderer_object.h"
#include "renderer_view.h"
#include "storage_mesh.h"
#include "storage_volume.h"

#include "buffers/drawing.h"
#include "buffers/ggx_f1_albedo.h"
#include "buffers/opacity.h"
#include "buffers/transparency.h"
#include "code/code.h"
#include "mesh/object.h"
#include "mesh/renderer.h"
#include "volume/object.h"
#include "volume/renderer.h"

#include <src/gpu/render_buffers.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <memory>
#include <optional>
#include <thread>
#include <vector>

namespace ns::gpu::renderer
{
class RendererImpl final : public Renderer, RendererViewEvents, StorageMeshEvents, StorageVolumeEvents
{
        const std::thread::id thread_id_ = std::this_thread::get_id();

        numerical::Region<2, int> viewport_;

        const vulkan::Device* const device_;
        const bool ray_tracing_;

        const vulkan::CommandPool* const graphics_command_pool_;
        const vulkan::Queue* const graphics_queue_;
        const vulkan::CommandPool* const transfer_command_pool_;
        const vulkan::Queue* const transfer_queue_;
        const vulkan::CommandPool* const compute_command_pool_;
        const vulkan::Queue* const compute_queue_;

        const RenderBuffers3D* render_buffers_ = nullptr;
        const vulkan::ImageWithMemory* object_image_ = nullptr;

        DrawingBuffer drawing_buffer_;
        GgxF1Albedo ggx_f1_albedo_;
        TransparencyBuffers transparency_buffers_;
        OpacityBuffers opacity_buffers_;
        std::unique_ptr<vulkan::DepthImageWithMemory> depth_copy_image_;

        MeshRenderer mesh_renderer_;
        VolumeRenderer volume_renderer_;

        const std::vector<vulkan::DescriptorSetLayoutAndBindings> mesh_layouts_{mesh_renderer_.mesh_layouts()};
        const std::vector<vulkan::DescriptorSetLayoutAndBindings> mesh_material_layouts_{
                mesh_renderer_.material_layouts()};
        const std::vector<vulkan::DescriptorSetLayoutAndBindings> volume_image_layouts_{
                volume_renderer_.image_layouts()};

        StorageMesh mesh_storage_;
        StorageVolume volume_storage_;
        std::optional<AccelerationStructure> acceleration_structure_;

        RendererObject renderer_object_;
        RendererView renderer_view_;

        RendererDraw renderer_draw_;

        void info(info::Functionality* functionality) const;
        void info(info::Description* description) const;

        void receive(const Info& info) const override;

        void cmd(const ObjectCommand& command);
        void cmd(const ViewCommand& command);

        void exec(const Command& command) override;

        VkSemaphore draw(
                VkSemaphore semaphore,
                const vulkan::Queue& graphics_queue_1,
                const vulkan::Queue& graphics_queue_2,
                unsigned index) const override;

        bool empty() const override;

        void create_buffers(
                RenderBuffers3D* render_buffers,
                const vulkan::ImageWithMemory* objects,
                const numerical::Region<2, int>& viewport) override;

        void delete_buffers() override;

        void create_depth_copy_image();

        void create_transparency_buffers();
        void create_opacity_buffers();
        void delete_mesh_shadow_mapping_buffers();
        void create_mesh_shadow_mapping_buffers();
        void create_mesh_render_command_buffers();
        void create_mesh_shadow_mapping_command_buffers();
        void create_mesh_command_buffers();
        void create_volume_command_buffers();

        void set_volume_matrix();

        void acceleration_structure_create();
        void acceleration_structure_update_matrices() const;

        // StorageMeshEvents

        std::unique_ptr<MeshObject> mesh_create() override;
        void mesh_visibility_changed() override;
        void mesh_visible_changed(const MeshObject::UpdateChanges& update_changes) override;

        // StorageVolumeEvents

        std::unique_ptr<VolumeObject> volume_create() override;
        void volume_visibility_changed() override;
        void volume_visible_changed(const VolumeObject::UpdateChanges& update_changes) override;

        // RendererViewEvents

        void view_show_normals_changed() override;
        void view_shadow_zoom_changed() override;
        void view_matrices_changed() override;
        void view_clip_plane_changed(bool visibility_changed) override;
        void view_show_clip_plane_lines_changed() override;

public:
        RendererImpl(
                const vulkan::Device* device,
                const Code& code,
                const vulkan::CommandPool* graphics_command_pool,
                const vulkan::Queue* graphics_queue,
                const vulkan::CommandPool* transfer_command_pool,
                const vulkan::Queue* transfer_queue,
                const vulkan::CommandPool* compute_command_pool,
                const vulkan::Queue* compute_queue,
                bool sample_shading,
                bool sampler_anisotropy);

        ~RendererImpl() override;

        RendererImpl(const RendererImpl&) = delete;
        RendererImpl& operator=(const RendererImpl&) = delete;
        RendererImpl(RendererImpl&&) = delete;
        RendererImpl& operator=(RendererImpl&&) = delete;
};
}
