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

#include "view_process.h"

#include "clear_buffer.h"

#include <src/color/color.h>
#include <src/gpu/renderer/event.h>
#include <src/gpu/renderer/renderer.h>
#include <src/gpu/text_writer/view.h>
#include <src/numerical/vector.h>
#include <src/view/com/camera.h>
#include <src/view/event.h>

#include <functional>
#include <utility>
#include <variant>

namespace ns::view::view
{
void ViewProcess::cmd(const command::UpdateMeshObject& v)
{
        if (const auto ptr = v.object.lock())
        {
                renderer_->exec(gpu::renderer::command::MeshUpdate(ptr.get()));
        }
}

void ViewProcess::cmd(const command::UpdateVolumeObject& v)
{
        if (const auto ptr = v.object.lock())
        {
                renderer_->exec(gpu::renderer::command::VolumeUpdate(ptr.get()));
        }
}

void ViewProcess::cmd(const command::DeleteObject& v)
{
        renderer_->exec(gpu::renderer::command::DeleteObject(v.id));
}

void ViewProcess::cmd(const command::DeleteAllObjects&)
{
        renderer_->exec(gpu::renderer::command::DeleteAllObjects());
        camera_->reset_view();
}

void ViewProcess::cmd(const command::ResetView&)
{
        camera_->reset_view();
}

void ViewProcess::cmd(const command::SetSampleCount& v)
{
        set_sample_count_(v.sample_count);
}

void ViewProcess::cmd(const command::SetLightingColor& v)
{
        renderer_->exec(gpu::renderer::command::SetLightingColor(v.value));
}

void ViewProcess::cmd(const command::SetFrontLightingProportion& v)
{
        renderer_->exec(gpu::renderer::command::SetFrontLightingProportion(v.proportion));
}

void ViewProcess::cmd(const command::SetBackgroundColor& v)
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

void ViewProcess::cmd(const command::SetClipPlaneColor& v)
{
        renderer_->exec(gpu::renderer::command::SetClipPlaneColor(v.value));
}

void ViewProcess::cmd(const command::SetWireframeColor& v)
{
        renderer_->exec(gpu::renderer::command::SetWireframeColor(v.value));
}

void ViewProcess::cmd(const command::SetNormalLength& v)
{
        renderer_->exec(gpu::renderer::command::SetNormalLength(v.value));
}

void ViewProcess::cmd(const command::SetNormalColorPositive& v)
{
        renderer_->exec(gpu::renderer::command::SetNormalColorPositive(v.value));
}

void ViewProcess::cmd(const command::SetNormalColorNegative& v)
{
        renderer_->exec(gpu::renderer::command::SetNormalColorNegative(v.value));
}

void ViewProcess::cmd(const command::SetFlatShading& v)
{
        renderer_->exec(gpu::renderer::command::SetFlatShading(v.flat_shading));
}

void ViewProcess::cmd(const command::ShowWireframe& v)
{
        renderer_->exec(gpu::renderer::command::SetShowWireframe(v.show));
}

void ViewProcess::cmd(const command::ShowShadow& v)
{
        renderer_->exec(gpu::renderer::command::SetShowShadow(v.show));
}

void ViewProcess::cmd(const command::ShowFog& v)
{
        renderer_->exec(gpu::renderer::command::SetShowFog(v.show));
}

void ViewProcess::cmd(const command::ShowMaterials& v)
{
        renderer_->exec(gpu::renderer::command::SetShowMaterials(v.show));
}

void ViewProcess::cmd(const command::ShowFps& v)
{
        text_active_ = v.show;
}

void ViewProcess::cmd(const command::ShowClipPlaneLines& v)
{
        renderer_->exec(gpu::renderer::command::SetShowClipPlaneLines(v.show));
}

void ViewProcess::cmd(const command::SetVerticalSync& v)
{
        if (v.enabled != vertical_sync_)
        {
                vertical_sync_ = v.enabled;
                create_swapchain_();
        }
}

void ViewProcess::cmd(const command::SetShadowZoom& v)
{
        renderer_->exec(gpu::renderer::command::SetShadowZoom(v.value));
}

void ViewProcess::cmd(const command::ShowNormals& v)
{
        renderer_->exec(gpu::renderer::command::SetShowNormals(v.show));
}

void ViewProcess::cmd(const command::WindowResize&)
{
}

ViewProcess::ViewProcess(
        ClearBuffer* const clear_buffer,
        gpu::renderer::Renderer* const renderer,
        gpu::text_writer::View* const text,
        com::Camera* const camera,
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

void ViewProcess::exec(const ViewCommand& command)
{
        std::visit(
                [this](const auto& v)
                {
                        cmd(v);
                },
                command);
}

bool ViewProcess::vertical_sync() const
{
        return vertical_sync_;
}

bool ViewProcess::text_active() const
{
        return text_active_;
}

numerical::Vector3f ViewProcess::clear_color_rgb32() const
{
        return clear_color_rgb32_;
}
}
