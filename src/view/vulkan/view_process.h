/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "../com/camera.h"
#include "../event.h"

#include <src/gpu/renderer/renderer.h>
#include <src/gpu/text_writer/view.h>

#include <functional>

namespace ns::view
{
class ViewProcess
{
        gpu::renderer::Renderer* renderer_;
        gpu::text_writer::View* text_;
        Camera* camera_;
        std::function<void()> create_swapchain_;
        bool vertical_sync_;
        bool text_active_ = true;

        void command(const command::UpdateMeshObject& v)
        {
                if (const auto ptr = v.object.lock())
                {
                        renderer_->object_update(*ptr);
                }
        }

        void command(const command::UpdateVolumeObject& v)
        {
                if (const auto ptr = v.object.lock())
                {
                        renderer_->object_update(*ptr);
                }
        }

        void command(const command::DeleteObject& v)
        {
                renderer_->object_delete(v.id);
        }

        void command(const command::ShowObject& v)
        {
                renderer_->object_show(v.id, v.show);
        }

        void command(const command::DeleteAllObjects&)
        {
                renderer_->object_delete_all();
                command(command::ResetView());
        }

        void command(const command::ResetView&)
        {
                camera_->reset(Vector3d(1, 0, 0), Vector3d(0, 1, 0), 1, Vector2d(0, 0));
        }

        void command(const command::SetLightingColor& v)
        {
                renderer_->set_lighting_color(v.value);
        }

        void command(const command::SetBackgroundColor& v)
        {
                renderer_->set_background_color(v.value);
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

        void command(const command::SetWireframeColor& v)
        {
                renderer_->set_wireframe_color(v.value);
        }

        void command(const command::SetNormalLength& v)
        {
                renderer_->set_normal_length(v.value);
        }

        void command(const command::SetNormalColorPositive& v)
        {
                renderer_->set_normal_color_positive(v.value);
        }

        void command(const command::SetNormalColorNegative& v)
        {
                renderer_->set_normal_color_negative(v.value);
        }

        void command(const command::ShowSmooth& v)
        {
                renderer_->set_show_smooth(v.show);
        }

        void command(const command::ShowWireframe& v)
        {
                renderer_->set_show_wireframe(v.show);
        }

        void command(const command::ShowShadow& v)
        {
                renderer_->set_show_shadow(v.show);
        }

        void command(const command::ShowFog& v)
        {
                renderer_->set_show_fog(v.show);
        }

        void command(const command::ShowMaterials& v)
        {
                renderer_->set_show_materials(v.show);
        }

        void command(const command::ShowFps& d)
        {
                text_active_ = d.show;
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
                renderer_->set_shadow_zoom(v.value);
        }

        void command(const command::ShowNormals& v)
        {
                renderer_->set_show_normals(v.show);
        }

        void command(const command::WindowResize&)
        {
        }

public:
        ViewProcess(
                gpu::renderer::Renderer* const renderer,
                gpu::text_writer::View* const text,
                Camera* const camera,
                const bool vertical_sync,
                std::function<void()> create_swapchain)
                : renderer_(renderer),
                  text_(text),
                  camera_(camera),
                  create_swapchain_(std::move(create_swapchain)),
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

        bool vertical_sync() const
        {
                return vertical_sync_;
        }

        bool text_active() const
        {
                return text_active_;
        }
};
}
