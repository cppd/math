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
#include <src/image/image.h>
#include <src/model/mesh_object.h>
#include <src/model/volume_object.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vec.h>
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
        explicit DeleteObject(ObjectId id) : id(id)
        {
        }
};

struct ShowObject final
{
        ObjectId id;
        bool show;
        explicit ShowObject(ObjectId id, bool show) : id(id), show(show)
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

struct SetClipPlaneColor final
{
        color::Color value;
        explicit SetClipPlaneColor(const color::Color& value) : value(value)
        {
        }
};

struct SetNormalLength final
{
        float value;
        explicit SetNormalLength(float value) : value(value)
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
        explicit ShowSmooth(bool show) : show(show)
        {
        }
};

struct ShowWireframe final
{
        bool show;
        explicit ShowWireframe(bool show) : show(show)
        {
        }
};

struct ShowShadow final
{
        bool show;
        explicit ShowShadow(bool show) : show(show)
        {
        }
};

struct ShowFog final
{
        bool show;
        explicit ShowFog(bool show) : show(show)
        {
        }
};

struct ShowMaterials final
{
        bool show;
        explicit ShowMaterials(bool show) : show(show)
        {
        }
};

struct ShowFps final
{
        bool show;
        explicit ShowFps(bool show) : show(show)
        {
        }
};

struct ShowPencilSketch final
{
        bool show;
        explicit ShowPencilSketch(bool show) : show(show)
        {
        }
};

struct ShowDft final
{
        bool show;
        explicit ShowDft(bool show) : show(show)
        {
        }
};

struct SetDftBrightness final
{
        double value;
        explicit SetDftBrightness(double value) : value(value)
        {
        }
};

struct SetDftBackgroundColor final
{
        color::Color value;
        explicit SetDftBackgroundColor(const color::Color& value) : value(value)
        {
        }
};

struct SetDftColor final
{
        color::Color value;
        explicit SetDftColor(const color::Color& value) : value(value)
        {
        }
};

struct ShowConvexHull2D final
{
        bool show;
        explicit ShowConvexHull2D(bool show) : show(show)
        {
        }
};

struct ShowOpticalFlow final
{
        bool show;
        explicit ShowOpticalFlow(bool show) : show(show)
        {
        }
};

struct SetVerticalSync final
{
        bool enabled;
        explicit SetVerticalSync(bool enabled) : enabled(enabled)
        {
        }
};

struct SetShadowZoom final
{
        double value;
        explicit SetShadowZoom(double value) : value(value)
        {
        }
};

struct ClipPlaneShow final
{
        double position;
        explicit ClipPlaneShow(double position) : position(position)
        {
        }
};

struct ClipPlanePosition final
{
        double position;
        explicit ClipPlanePosition(double position) : position(position)
        {
        }
};

struct ClipPlaneHide final
{
};

struct ShowNormals final
{
        bool show;
        explicit ShowNormals(bool show) : show(show)
        {
        }
};

struct MousePress final
{
        int x, y;
        MouseButton button;
        MousePress(int x, int y, MouseButton button) : x(x), y(y), button(button)
        {
        }
};

struct MouseRelease final
{
        int x, y;
        MouseButton button;
        MouseRelease(int x, int y, MouseButton button) : x(x), y(y), button(button)
        {
        }
};

struct MouseMove final
{
        int x, y;
        MouseMove(int x, int y) : x(x), y(y)
        {
        }
};

struct MouseWheel final
{
        int x, y;
        double delta;
        MouseWheel(int x, int y, double delta) : x(x), y(y), delta(delta)
        {
        }
};

struct WindowResize final
{
        int x, y;
        WindowResize(int x, int y) : x(x), y(y)
        {
        }
};
}

using Command = std::variant<
        command::ClipPlaneHide,
        command::ClipPlanePosition,
        command::ClipPlaneShow,
        command::DeleteAllObjects,
        command::DeleteObject,
        command::MouseMove,
        command::MousePress,
        command::MouseRelease,
        command::MouseWheel,
        command::ResetView,
        command::SetBackgroundColor,
        command::SetClipPlaneColor,
        command::SetDftBackgroundColor,
        command::SetDftBrightness,
        command::SetDftColor,
        command::SetLightingColor,
        command::SetNormalColorNegative,
        command::SetNormalColorPositive,
        command::SetNormalLength,
        command::SetShadowZoom,
        command::SetVerticalSync,
        command::SetWireframeColor,
        command::ShowConvexHull2D,
        command::ShowDft,
        command::ShowFog,
        command::ShowFps,
        command::ShowMaterials,
        command::ShowNormals,
        command::ShowObject,
        command::ShowOpticalFlow,
        command::ShowPencilSketch,
        command::ShowShadow,
        command::ShowSmooth,
        command::ShowWireframe,
        command::UpdateMeshObject,
        command::UpdateVolumeObject,
        command::WindowResize>;

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
