/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "event.h"

#include "buffers/drawing.h"

#include <src/com/error.h>
#include <src/numerical/matrix.h>
#include <src/numerical/transform.h>
#include <src/numerical/vector.h>

#include <optional>
#include <variant>

namespace ns::gpu::renderer
{
class RendererViewEvents
{
protected:
        ~RendererViewEvents() = default;

public:
        virtual void view_show_normals_changed() = 0;
        virtual void view_matrices_changed() = 0;
        virtual void view_clip_plane_changed(bool visibility_changed) = 0;
        virtual void view_show_clip_plane_lines_changed() = 0;
        virtual void view_shadow_zoom_changed() = 0;
};

class RendererView final
{
        const bool shadow_mapping_;
        DrawingBuffer* const drawing_buffer_;
        RendererViewEvents* const events_;

        Matrix4d vp_matrix_ = IDENTITY_MATRIX<4, double>;
        Matrix4d shadow_vp_matrix_ = IDENTITY_MATRIX<4, double>;
        Matrix4d world_to_shadow_matrix_ = IDENTITY_MATRIX<4, double>;

        double shadow_zoom_ = 1;
        bool show_shadow_ = false;
        std::optional<Vector4d> clip_plane_;
        bool show_clip_plane_lines_ = true;
        bool show_normals_ = false;

        [[nodiscard]] static Matrix4d camera_volume_to_projection(const CameraInfo::Volume& volume)
        {
                return numerical::transform::ortho_vulkan<double>(
                        volume.left, volume.right, volume.bottom, volume.top, volume.near, volume.far);
        }

        void cmd(const command::SetLightingColor& v)
        {
                drawing_buffer_->set_lighting_color(v.color.rgb32().max_n(0));
        }

        void cmd(const command::SetFrontLightingProportion& v)
        {
                ASSERT(v.proportion >= 0 && v.proportion <= 1);
                drawing_buffer_->set_lighting_proportions(v.proportion, 1 - v.proportion);
        }

        void cmd(const command::SetBackgroundColor& v)
        {
                drawing_buffer_->set_background_color(v.color.rgb32().clamp(0, 1));
        }

        void cmd(const command::SetWireframeColor& v)
        {
                drawing_buffer_->set_wireframe_color(v.color.rgb32().clamp(0, 1));
        }

        void cmd(const command::SetClipPlaneColor& v)
        {
                drawing_buffer_->set_clip_plane_color(v.color.rgb32().clamp(0, 1));
        }

        void cmd(const command::SetNormalLength& v)
        {
                drawing_buffer_->set_normal_length(v.length);
        }

        void cmd(const command::SetNormalColorPositive& v)
        {
                drawing_buffer_->set_normal_color_positive(v.color.rgb32().clamp(0, 1));
        }

        void cmd(const command::SetNormalColorNegative& v)
        {
                drawing_buffer_->set_normal_color_negative(v.color.rgb32().clamp(0, 1));
        }

        void cmd(const command::SetFlatShading& v)
        {
                drawing_buffer_->set_flat_shading(v.flat_shading);
        }

        void cmd(const command::SetShowWireframe& v)
        {
                drawing_buffer_->set_show_wireframe(v.show);
        }

        void cmd(const command::SetShowShadow& v)
        {
                drawing_buffer_->set_show_shadow(v.show);
                show_shadow_ = v.show;
        }

        void cmd(const command::SetShowFog& v)
        {
                drawing_buffer_->set_show_fog(v.show);
        }

        void cmd(const command::SetShowMaterials& v)
        {
                drawing_buffer_->set_show_materials(v.show);
        }

        void cmd(const command::SetShowNormals& v)
        {
                if (show_normals_ != v.show)
                {
                        show_normals_ = v.show;
                        events_->view_show_normals_changed();
                }
        }

        void cmd(const command::SetShadowZoom& v)
        {
                if (!shadow_mapping_)
                {
                        return;
                }
                if (shadow_zoom_ != v.zoom)
                {
                        shadow_zoom_ = v.zoom;
                        events_->view_shadow_zoom_changed();
                }
        }

        void cmd(const command::SetCamera& v)
        {
                const CameraInfo& c = *v.info;

                vp_matrix_ = camera_volume_to_projection(c.main_volume) * c.main_view_matrix;
                drawing_buffer_->set_matrix(vp_matrix_);

                if (shadow_mapping_)
                {
                        // shadow coordinates x(-1, 1) y(-1, 1) z(0, 1).
                        // shadow texture coordinates x(0, 1) y(0, 1) z(0, 1).
                        static constexpr Matrix4d TEXTURE_MATRIX =
                                numerical::transform::scale<double>(0.5, 0.5, 1)
                                * numerical::transform::translate<double>(1, 1, 0);

                        shadow_vp_matrix_ = camera_volume_to_projection(c.shadow_volume) * c.shadow_view_matrix;
                        world_to_shadow_matrix_ = TEXTURE_MATRIX * shadow_vp_matrix_;
                }

                drawing_buffer_->set_direction_to_light(-to_vector<float>(c.light_direction));
                drawing_buffer_->set_direction_to_camera(-to_vector<float>(c.camera_direction));

                events_->view_matrices_changed();
        }

        void cmd(const command::SetClipPlane& v)
        {
                if (clip_plane_ == v.plane)
                {
                        return;
                }
                const bool visibility_changed = clip_plane_.has_value() != v.plane.has_value();
                clip_plane_ = v.plane;
                if (clip_plane_)
                {
                        drawing_buffer_->set_clip_plane(*clip_plane_, true);
                }
                else
                {
                        drawing_buffer_->set_clip_plane(Vector4d(0), false);
                }
                events_->view_clip_plane_changed(visibility_changed);
        }

        void cmd(const command::SetShowClipPlaneLines& v)
        {
                if (show_clip_plane_lines_ == v.show)
                {
                        return;
                }
                show_clip_plane_lines_ = v.show;
                events_->view_show_clip_plane_lines_changed();
        }

public:
        RendererView(const bool shadow_mapping, DrawingBuffer* const drawing_buffer, RendererViewEvents* const events)
                : shadow_mapping_(shadow_mapping),
                  drawing_buffer_(drawing_buffer),
                  events_(events)
        {
        }

        void exec(const ViewCommand& command)
        {
                std::visit(
                        [this](const auto& v)
                        {
                                cmd(v);
                        },
                        command);
        }

        [[nodiscard]] bool show_shadow() const
        {
                return show_shadow_;
        }

        [[nodiscard]] const std::optional<Vector4d>& clip_plane() const
        {
                return clip_plane_;
        }

        [[nodiscard]] bool show_clip_plane_lines() const
        {
                return show_clip_plane_lines_;
        }

        [[nodiscard]] bool show_normals() const
        {
                return show_normals_;
        }

        [[nodiscard]] const Matrix4d& vp_matrix() const
        {
                return vp_matrix_;
        }

        [[nodiscard]] const Matrix4d& shadow_vp_matrix() const
        {
                return shadow_vp_matrix_;
        }

        [[nodiscard]] const Matrix4d& world_to_shadow_matrix() const
        {
                return world_to_shadow_matrix_;
        }

        [[nodiscard]] double shadow_zoom() const
        {
                ASSERT(shadow_mapping_);
                return shadow_zoom_;
        }
};
}
