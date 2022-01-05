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

#include <src/color/color.h>
#include <src/image/image.h>
#include <src/model/mesh_object.h>
#include <src/model/volume_object.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/window/handle.h>

#include <variant>

namespace ns::view
{
enum class MouseButton
{
        LEFT,
        RIGHT
};

namespace command
{
struct UpdateMeshObject final
{
        std::weak_ptr<const mesh::MeshObject<3>> object;
        explicit UpdateMeshObject(std::weak_ptr<const mesh::MeshObject<3>> object) : object(std::move(object))
        {
        }
};

struct UpdateVolumeObject final
{
        std::weak_ptr<const volume::VolumeObject<3>> object;
        explicit UpdateVolumeObject(std::weak_ptr<const volume::VolumeObject<3>> object) : object(std::move(object))
        {
        }
};

struct DeleteObject final
{
        ObjectId id;
        explicit DeleteObject(const ObjectId id) : id(id)
        {
        }
};

struct DeleteAllObjects final
{
};

struct ResetView final
{
};

struct SetLightingColor final
{
        color::Color value;
        explicit SetLightingColor(const color::Color& value) : value(value)
        {
        }
};

struct SetBackgroundColor final
{
        color::Color value;
        explicit SetBackgroundColor(const color::Color& value) : value(value)
        {
        }
};

struct SetWireframeColor final
{
        color::Color value;
        explicit SetWireframeColor(const color::Color& value) : value(value)
        {
        }
};

struct SetNormalLength final
{
        float value;
        explicit SetNormalLength(const float value) : value(value)
        {
        }
};

struct SetNormalColorPositive final
{
        color::Color value;
        explicit SetNormalColorPositive(const color::Color& value) : value(value)
        {
        }
};

struct SetNormalColorNegative final
{
        color::Color value;
        explicit SetNormalColorNegative(const color::Color& value) : value(value)
        {
        }
};

struct ShowSmooth final
{
        bool show;
        explicit ShowSmooth(const bool show) : show(show)
        {
        }
};

struct ShowWireframe final
{
        bool show;
        explicit ShowWireframe(const bool show) : show(show)
        {
        }
};

struct ShowShadow final
{
        bool show;
        explicit ShowShadow(const bool show) : show(show)
        {
        }
};

struct ShowFog final
{
        bool show;
        explicit ShowFog(const bool show) : show(show)
        {
        }
};

struct ShowMaterials final
{
        bool show;
        explicit ShowMaterials(const bool show) : show(show)
        {
        }
};

struct ShowFps final
{
        bool show;
        explicit ShowFps(const bool show) : show(show)
        {
        }
};

struct SetVerticalSync final
{
        bool enabled;
        explicit SetVerticalSync(const bool enabled) : enabled(enabled)
        {
        }
};

struct SetShadowZoom final
{
        double value;
        explicit SetShadowZoom(const double value) : value(value)
        {
        }
};

struct ShowNormals final
{
        bool show;
        explicit ShowNormals(const bool show) : show(show)
        {
        }
};

struct WindowResize final
{
        int x, y;
        WindowResize(const int x, const int y) : x(x), y(y)
        {
        }
};

struct ClipPlaneSetColor final
{
        color::Color value;
        explicit ClipPlaneSetColor(const color::Color& value) : value(value)
        {
        }
};

struct ClipPlaneShow final
{
        double position;
        explicit ClipPlaneShow(const double position) : position(position)
        {
        }
};

struct ClipPlaneSetPosition final
{
        double position;
        explicit ClipPlaneSetPosition(const double position) : position(position)
        {
        }
};

struct ClipPlaneHide final
{
};

struct MousePress final
{
        int x, y;
        MouseButton button;
        MousePress(const int x, const int y, const MouseButton button) : x(x), y(y), button(button)
        {
        }
};

struct MouseRelease final
{
        int x, y;
        MouseButton button;
        MouseRelease(const int x, const int y, const MouseButton button) : x(x), y(y), button(button)
        {
        }
};

struct MouseMove final
{
        int x, y;
        MouseMove(const int x, const int y) : x(x), y(y)
        {
        }
};

struct MouseWheel final
{
        int x, y;
        double delta;
        MouseWheel(const int x, const int y, const double delta) : x(x), y(y), delta(delta)
        {
        }
};

struct DftShow final
{
        bool show;
        explicit DftShow(const bool show) : show(show)
        {
        }
};

struct DftSetBrightness final
{
        double value;
        explicit DftSetBrightness(const double value) : value(value)
        {
        }
};

struct DftSetBackgroundColor final
{
        color::Color value;
        explicit DftSetBackgroundColor(const color::Color& value) : value(value)
        {
        }
};

struct DftSetColor final
{
        color::Color value;
        explicit DftSetColor(const color::Color& value) : value(value)
        {
        }
};

struct ConvexHullShow final
{
        bool show;
        explicit ConvexHullShow(const bool show) : show(show)
        {
        }
};

struct OpticalFlowShow final
{
        bool show;
        explicit OpticalFlowShow(const bool show) : show(show)
        {
        }
};

struct PencilSketchShow final
{
        bool show;
        explicit PencilSketchShow(const bool show) : show(show)
        {
        }
};
}

using ClipPlaneCommand = std::variant<
        command::ClipPlaneHide,
        command::ClipPlaneSetColor,
        command::ClipPlaneSetPosition,
        command::ClipPlaneShow>;

using ImageCommand = std::variant<
        command::ConvexHullShow,
        command::DftSetBackgroundColor,
        command::DftSetBrightness,
        command::DftSetColor,
        command::DftShow,
        command::OpticalFlowShow,
        command::PencilSketchShow>;

using MouseCommand = std::variant<command::MouseMove, command::MousePress, command::MouseRelease, command::MouseWheel>;

using ViewCommand = std::variant<
        command::DeleteAllObjects,
        command::DeleteObject,
        command::ResetView,
        command::SetBackgroundColor,
        command::SetLightingColor,
        command::SetNormalColorNegative,
        command::SetNormalColorPositive,
        command::SetNormalLength,
        command::SetShadowZoom,
        command::SetVerticalSync,
        command::SetWireframeColor,
        command::ShowFog,
        command::ShowFps,
        command::ShowMaterials,
        command::ShowNormals,
        command::ShowShadow,
        command::ShowSmooth,
        command::ShowWireframe,
        command::UpdateMeshObject,
        command::UpdateVolumeObject,
        command::WindowResize>;

using Command = std::variant<ClipPlaneCommand, ImageCommand, MouseCommand, ViewCommand>;

namespace info
{
struct Camera final
{
        Vector3d up;
        Vector3d forward;
        Vector3d lighting;
        Vector3d view_center;
        double view_width;
        int width;
        int height;
};

struct Image final
{
        image::Image<2> image;
};
}

using Info = std::variant<info::Camera*, info::Image*>;
}
