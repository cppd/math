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

#include <src/color/color.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vec.h>

#include <optional>
#include <variant>

namespace ns::gpu::renderer
{
struct CameraInfo final
{
        struct Volume final
        {
                double left, right, bottom, top, near, far;
        };

        Volume main_volume;
        Volume shadow_volume;
        Matrix4d main_view_matrix;
        Matrix4d shadow_view_matrix;
        Vector3d light_direction;
        Vector3d camera_direction;
};

struct SetLightingColor final
{
        color::Color color;
        explicit SetLightingColor(const color::Color& color) : color(color)
        {
        }
};

struct SetBackgroundColor final
{
        color::Color color;
        explicit SetBackgroundColor(const color::Color& color) : color(color)
        {
        }
};

struct SetWireframeColor final
{
        color::Color color;
        explicit SetWireframeColor(const color::Color& color) : color(color)
        {
        }
};

struct SetClipPlaneColor final
{
        color::Color color;
        explicit SetClipPlaneColor(const color::Color& color) : color(color)
        {
        }
};

struct SetNormalColorPositive final
{
        color::Color color;
        explicit SetNormalColorPositive(const color::Color& color) : color(color)
        {
        }
};

struct SetNormalColorNegative final
{
        color::Color color;
        explicit SetNormalColorNegative(const color::Color& color) : color(color)
        {
        }
};

struct SetCamera final
{
        CameraInfo info;
        explicit SetCamera(const CameraInfo& info) : info(info)
        {
        }
};

struct SetClipPlane final
{
        std::optional<Vector4d> plane;
        explicit SetClipPlane(const std::optional<Vector4d>& plane) : plane(plane)
        {
        }
};

struct SetNormalLength final
{
        float length;
        explicit SetNormalLength(const float length) : length(length)
        {
        }
};

struct SetShowSmooth final
{
        bool show;
        explicit SetShowSmooth(const bool show) : show(show)
        {
        }
};

struct SetShowWireframe final
{
        bool show;
        explicit SetShowWireframe(const bool show) : show(show)
        {
        }
};

struct SetShowShadow final
{
        bool show;
        explicit SetShowShadow(const bool show) : show(show)
        {
        }
};

struct SetShowFog final
{
        bool show;
        explicit SetShowFog(const bool show) : show(show)
        {
        }
};

struct SetShowMaterials final
{
        bool show;
        explicit SetShowMaterials(const bool show) : show(show)
        {
        }
};

struct SetShowNormals final
{
        bool show;
        explicit SetShowNormals(const bool show) : show(show)
        {
        }
};

struct SetShadowZoom final
{
        double zoom;
        explicit SetShadowZoom(const double zoom) : zoom(zoom)
        {
        }
};

using Command = std::variant<
        SetBackgroundColor,
        SetCamera,
        SetClipPlane,
        SetClipPlaneColor,
        SetLightingColor,
        SetNormalColorNegative,
        SetNormalColorPositive,
        SetNormalLength,
        SetShadowZoom,
        SetShowFog,
        SetShowMaterials,
        SetShowNormals,
        SetShowShadow,
        SetShowSmooth,
        SetShowWireframe,
        SetWireframeColor>;
}
