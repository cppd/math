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

#include "renderer_command.h"

#include "shaders/buffers.h"

#include <src/numerical/matrix.h>
#include <src/numerical/region.h>
#include <src/numerical/transform.h>
#include <src/numerical/vec.h>

namespace ns::gpu::renderer
{

class RendererProcessEvents
{
protected:
        ~RendererProcessEvents() = default;

public:
        virtual void background_changed() = 0;
        virtual void show_normals_changed() = 0;
        virtual void shadow_zoom_changed() = 0;
        virtual void matrices_changed() = 0;
        virtual void clip_plane_changed() = 0;
};

class RendererProcess final
{
        // shadow coordinates x(-1, 1) y(-1, 1) z(0, 1).
        // shadow texture coordinates x(0, 1) y(0, 1) z(0, 1).
        static constexpr Matrix4d SHADOW_TEXTURE_MATRIX =
                matrix::scale<double>(0.5, 0.5, 1) * matrix::translate<double>(1, 1, 0);

        Matrix4d main_vp_matrix_ = Matrix4d(1);
        Matrix4d shadow_vp_matrix_ = Matrix4d(1);
        Matrix4d shadow_vp_texture_matrix_ = Matrix4d(1);

        Vector3f clear_color_rgb32_ = Vector3f(0);
        double shadow_zoom_ = 1;
        bool show_shadow_ = false;
        std::optional<Vector4d> clip_plane_;
        bool show_normals_ = false;

        ShaderBuffers* shader_buffers_;
        RendererProcessEvents* events_;

        void command(const SetLightingColor& v)
        {
                shader_buffers_->set_lighting_color(v.color.rgb32().max_n(0));
        }

        void command(const SetBackgroundColor& v)
        {
                clear_color_rgb32_ = v.color.rgb32().clamp(0, 1);
                shader_buffers_->set_background_color(clear_color_rgb32_);
                events_->background_changed();
        }

        void command(const SetWireframeColor& v)
        {
                shader_buffers_->set_wireframe_color(v.color.rgb32().clamp(0, 1));
        }

        void command(const SetClipPlaneColor& v)
        {
                shader_buffers_->set_clip_plane_color(v.color.rgb32().clamp(0, 1));
        }

        void command(const SetNormalLength& v)
        {
                shader_buffers_->set_normal_length(v.length);
        }

        void command(const SetNormalColorPositive& v)
        {
                shader_buffers_->set_normal_color_positive(v.color.rgb32().clamp(0, 1));
        }

        void command(const SetNormalColorNegative& v)
        {
                shader_buffers_->set_normal_color_negative(v.color.rgb32().clamp(0, 1));
        }

        void command(const SetShowSmooth& v)
        {
                shader_buffers_->set_show_smooth(v.show);
        }

        void command(const SetShowWireframe& v)
        {
                shader_buffers_->set_show_wireframe(v.show);
        }

        void command(const SetShowShadow& v)
        {
                shader_buffers_->set_show_shadow(v.show);
                show_shadow_ = v.show;
        }

        void command(const SetShowFog& v)
        {
                shader_buffers_->set_show_fog(v.show);
        }

        void command(const SetShowMaterials& v)
        {
                shader_buffers_->set_show_materials(v.show);
        }

        void command(const SetShowNormals& v)
        {
                if (show_normals_ != v.show)
                {
                        show_normals_ = v.show;
                        events_->show_normals_changed();
                }
        }

        void command(const SetShadowZoom& v)
        {
                if (shadow_zoom_ != v.zoom)
                {
                        shadow_zoom_ = v.zoom;
                        events_->shadow_zoom_changed();
                }
        }

        void command(const SetCamera& v)
        {
                const CameraInfo& c = *v.info;

                const Matrix4d& shadow_projection_matrix = matrix::ortho_vulkan<double>(
                        c.shadow_volume.left, c.shadow_volume.right, c.shadow_volume.bottom, c.shadow_volume.top,
                        c.shadow_volume.near, c.shadow_volume.far);
                const Matrix4d& main_projection_matrix = matrix::ortho_vulkan<double>(
                        c.main_volume.left, c.main_volume.right, c.main_volume.bottom, c.main_volume.top,
                        c.main_volume.near, c.main_volume.far);

                shadow_vp_matrix_ = shadow_projection_matrix * c.shadow_view_matrix;
                shadow_vp_texture_matrix_ = SHADOW_TEXTURE_MATRIX * shadow_vp_matrix_;
                main_vp_matrix_ = main_projection_matrix * c.main_view_matrix;

                shader_buffers_->set_direction_to_light(-to_vector<float>(c.light_direction));
                shader_buffers_->set_direction_to_camera(-to_vector<float>(c.camera_direction));
                shader_buffers_->set_matrices(main_vp_matrix_, shadow_vp_matrix_, shadow_vp_texture_matrix_);

                events_->matrices_changed();
        }

        void command(const SetClipPlane& v)
        {
                clip_plane_ = v.plane;
                if (clip_plane_)
                {
                        shader_buffers_->set_clip_plane(*clip_plane_, true);
                }
                else
                {
                        shader_buffers_->set_clip_plane(Vector4d(0), false);
                }
                events_->clip_plane_changed();
        }

public:
        RendererProcess(ShaderBuffers* const shader_buffers, RendererProcessEvents* const events)
                : shader_buffers_(shader_buffers), events_(events)
        {
        }

        void exec(const Command& renderer_command)
        {
                const auto visitor = [this](const auto& v)
                {
                        command(v);
                };
                std::visit(visitor, renderer_command);
        }

        bool show_shadow() const
        {
                return show_shadow_;
        }

        double shadow_zoom() const
        {
                return shadow_zoom_;
        }

        Vector3f clear_color_rgb32() const
        {
                return clear_color_rgb32_;
        }

        const std::optional<Vector4d>& clip_plane() const
        {
                return clip_plane_;
        }

        bool show_normals() const
        {
                return show_normals_;
        }

        const Matrix4d& main_vp_matrix() const
        {
                return main_vp_matrix_;
        }
};
}
