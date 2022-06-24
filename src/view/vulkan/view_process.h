/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "clear_buffer.h"

#include "../com/camera.h"
#include "../event.h"

#include <src/gpu/renderer/renderer.h>
#include <src/gpu/text_writer/view.h>

#include <functional>

namespace ns::view
{
class ViewProcess final
{
        ClearBuffer* clear_buffer_;
        gpu::renderer::Renderer* renderer_;
        gpu::text_writer::View* text_;
        Camera* camera_;
        std::function<void()> create_swapchain_;
        std::function<void(int)> set_sample_count_;
        bool vertical_sync_;
        bool text_active_ = true;
        Vector3f clear_color_rgb32_ = Vector3f(0);

        void command(const command::UpdateMeshObject& v)
        {
                if (const auto ptr = v.object.lock())
                {
                        renderer_->exec(gpu::renderer::command::MeshUpdate(ptr.get()));
                }
        }

        void command(const command::UpdateVolumeObject& v)
        {
                if (const auto ptr = v.object.lock())
                {
                        renderer_->exec(gpu::renderer::command::VolumeUpdate(ptr.get()));
                }
        }

        void command(const command::DeleteObject& v)
        {
                renderer_->exec(gpu::renderer::command::DeleteObject(v.id));
        }

        void command(const command::DeleteAllObjects&)
        {
                renderer_->exec(gpu::renderer::command::DeleteAllObjects());
                command(command::ResetView());
        }

        void command(const command::ResetView&)
        {
                camera_->reset(Vector3d(1, 0, 0), Vector3d(0, 1, 0), 1, Vector2d(0, 0));
        }

        void command(const command::SetSampleCount& v)
        {
                set_sample_count_(v.sample_count);
        }

        void command(const command::SetLightingColor& v)
        {
                renderer_->exec(gpu::renderer::command::SetLightingColor(v.value));
        }

        void command(const command::SetFrontLightingProportion& v)
        {
                renderer_->exec(gpu::renderer::command::SetFrontLightingProportion(v.proportion));
        }

        void command(const command::SetBackgroundColor& v)
        {
                clear_color_rgb32_ = v.value.rgb32().clamp(0, 1);
                clear_buffer_->set_color(clear_color_rgb32_);
                renderer_->exec(gpu::renderer::command::SetBackgroundColor(v.value));
                const bool background_is_dark = v.value.luminance() <= 0.5;
                if (background_is_dark)
                {
                        static constexpr color::Color WHITE(1);
                        text_->set_color(WHITE);
                }
                else
                {
                        static constexpr color::Color BLACK(0);
                        text_->set_color(BLACK);
                }
        }

        void command(const command::SetClipPlaneColor& v)
        {
                renderer_->exec(gpu::renderer::command::SetClipPlaneColor(v.value));
        }

        void command(const command::SetWireframeColor& v)
        {
                renderer_->exec(gpu::renderer::command::SetWireframeColor(v.value));
        }

        void command(const command::SetNormalLength& v)
        {
                renderer_->exec(gpu::renderer::command::SetNormalLength(v.value));
        }

        void command(const command::SetNormalColorPositive& v)
        {
                renderer_->exec(gpu::renderer::command::SetNormalColorPositive(v.value));
        }

        void command(const command::SetNormalColorNegative& v)
        {
                renderer_->exec(gpu::renderer::command::SetNormalColorNegative(v.value));
        }

        void command(const command::ShowSmooth& v)
        {
                renderer_->exec(gpu::renderer::command::SetShowSmooth(v.show));
        }

        void command(const command::ShowWireframe& v)
        {
                renderer_->exec(gpu::renderer::command::SetShowWireframe(v.show));
        }

        void command(const command::ShowShadow& v)
        {
                renderer_->exec(gpu::renderer::command::SetShowShadow(v.show));
        }

        void command(const command::ShowFog& v)
        {
                renderer_->exec(gpu::renderer::command::SetShowFog(v.show));
        }

        void command(const command::ShowMaterials& v)
        {
                renderer_->exec(gpu::renderer::command::SetShowMaterials(v.show));
        }

        void command(const command::ShowFps& v)
        {
                text_active_ = v.show;
        }

        void command(const command::ShowClipPlaneLines& v)
        {
                renderer_->exec(gpu::renderer::command::SetShowClipPlaneLines(v.show));
        }

        void command(const command::SetVerticalSync& v)
        {
                if (v.enabled != vertical_sync_)
                {
                        vertical_sync_ = v.enabled;
                        create_swapchain_();
                }
        }

        void command(const command::SetShadowZoom& v)
        {
                renderer_->exec(gpu::renderer::command::SetShadowZoom(v.value));
        }

        void command(const command::ShowNormals& v)
        {
                renderer_->exec(gpu::renderer::command::SetShowNormals(v.show));
        }

        void command(const command::WindowResize&)
        {
        }

public:
        ViewProcess(
                ClearBuffer* const clear_buffer,
                gpu::renderer::Renderer* const renderer,
                gpu::text_writer::View* const text,
                Camera* const camera,
                const bool vertical_sync,
                std::function<void()> create_swapchain,
                std::function<void(int)> set_sample_count)
                : clear_buffer_(clear_buffer),
                  renderer_(renderer),
                  text_(text),
                  camera_(camera),
                  create_swapchain_(std::move(create_swapchain)),
                  set_sample_count_(std::move(set_sample_count)),
                  vertical_sync_(vertical_sync)
        {
        }

        void command(const ViewCommand& view_command)
        {
                const auto visitor = [this](const auto& v)
                {
                        command(v);
                };
                std::visit(visitor, view_command);
        }

        [[nodiscard]] bool vertical_sync() const
        {
                return vertical_sync_;
        }

        [[nodiscard]] bool text_active() const
        {
                return text_active_;
        }

        [[nodiscard]] Vector3f clear_color_rgb32() const
        {
                return clear_color_rgb32_;
        }
};
}
