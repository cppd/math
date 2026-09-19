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

#include "clear_buffer.h"

#include <src/gpu/renderer/renderer.h>
#include <src/gpu/text_writer/view.h>
#include <src/numerical/vector.h>
#include <src/view/com/camera.h>
#include <src/view/event.h>

#include <functional>

namespace ns::view::view
{
class ViewProcess final
{
        ClearBuffer* const clear_buffer_;
        gpu::renderer::Renderer* const renderer_;
        gpu::text_writer::View* const text_;
        com::Camera* const camera_;
        const std::function<void()> create_swapchain_;
        const std::function<void(int)> set_sample_count_;

        bool vertical_sync_;
        bool text_active_ = true;
        numerical::Vector3f clear_color_rgb32_ = numerical::Vector3f(0);

        void cmd(const command::UpdateMeshObject& v);
        void cmd(const command::UpdateVolumeObject& v);
        void cmd(const command::DeleteObject& v);
        void cmd(const command::DeleteAllObjects&);
        void cmd(const command::ResetView&);
        void cmd(const command::SetSampleCount& v);
        void cmd(const command::SetLightingColor& v);
        void cmd(const command::SetFrontLightingProportion& v);
        void cmd(const command::SetBackgroundColor& v);
        void cmd(const command::SetClipPlaneColor& v);
        void cmd(const command::SetWireframeColor& v);
        void cmd(const command::SetNormalLength& v);
        void cmd(const command::SetNormalColorPositive& v);
        void cmd(const command::SetNormalColorNegative& v);
        void cmd(const command::SetFlatShading& v);
        void cmd(const command::ShowWireframe& v);
        void cmd(const command::ShowShadow& v);
        void cmd(const command::ShowFog& v);
        void cmd(const command::ShowMaterials& v);
        void cmd(const command::ShowFps& v);
        void cmd(const command::ShowClipPlaneLines& v);
        void cmd(const command::SetVerticalSync& v);
        void cmd(const command::SetShadowZoom& v);
        void cmd(const command::ShowNormals& v);
        void cmd(const command::WindowResize&);

public:
        ViewProcess(
                ClearBuffer* clear_buffer,
                gpu::renderer::Renderer* renderer,
                gpu::text_writer::View* text,
                com::Camera* camera,
                bool vertical_sync,
                std::function<void()> create_swapchain,
                std::function<void(int)> set_sample_count);

        void exec(const ViewCommand& command);

        [[nodiscard]] bool vertical_sync() const;
        [[nodiscard]] bool text_active() const;
        [[nodiscard]] numerical::Vector3f clear_color_rgb32() const;
};
}
