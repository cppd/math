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

#include "colors_widget.h"

#include <src/color/color.h>
#include <src/gui/com/support.h>
#include <src/gui/dialogs/color_dialog.h>
#include <src/view/event.h>
#include <src/view/view.h>

#include <QColor>
#include <QPointer>
#include <QToolButton>
#include <QWidget>

#include <vector>

namespace ns::gui::main_window
{
namespace
{
constexpr QRgb BACKGROUND_COLOR = qRgb(75, 90, 105);
constexpr QRgb WIREFRAME_COLOR = qRgb(255, 255, 255);
constexpr QRgb CLIP_PLANE_COLOR = qRgb(255, 225, 100);
constexpr QRgb NORMAL_COLOR_POSITIVE = qRgb(70, 180, 120);
constexpr QRgb NORMAL_COLOR_NEGATIVE = qRgb(70, 120, 180);
constexpr QRgb DFT_BACKGROUND_COLOR = qRgb(0, 0, 50);
constexpr QRgb DFT_COLOR = qRgb(150, 200, 250);
}

ColorsWidget::ColorsWidget()
        : QWidget(nullptr)
{
        ui_.setupUi(this);

        set_background_color(BACKGROUND_COLOR);
        set_wireframe_color(WIREFRAME_COLOR);
        set_clip_plane_color(CLIP_PLANE_COLOR);
        set_normal_color_positive(NORMAL_COLOR_POSITIVE);
        set_normal_color_negative(NORMAL_COLOR_NEGATIVE);
        set_dft_background_color(DFT_BACKGROUND_COLOR);
        set_dft_color(DFT_COLOR);

        connect(ui_.tool_button_background_color, &QToolButton::clicked, this,
                &ColorsWidget::on_background_color_clicked);
        connect(ui_.tool_button_clip_plane_color, &QToolButton::clicked, this,
                &ColorsWidget::on_clip_plane_color_clicked);
        connect(ui_.tool_button_dft_background_color, &QToolButton::clicked, this,
                &ColorsWidget::on_dft_background_color_clicked);
        connect(ui_.tool_button_dft_color, &QToolButton::clicked, this, &ColorsWidget::on_dft_color_clicked);
        connect(ui_.tool_button_normal_color_negative, &QToolButton::clicked, this,
                &ColorsWidget::on_normal_color_negative_clicked);
        connect(ui_.tool_button_normal_color_positive, &QToolButton::clicked, this,
                &ColorsWidget::on_normal_color_positive_clicked);
        connect(ui_.tool_button_wireframe_color, &QToolButton::clicked, this,
                &ColorsWidget::on_wireframe_color_clicked);

        this->adjustSize();

        const auto h = ui_.tool_button_background_color->size().height();
        ui_.widget_background_color->setMinimumSize(h, h);
        ui_.widget_clip_plane_color->setMinimumSize(h, h);
        ui_.widget_dft_background_color->setMinimumSize(h, h);
        ui_.widget_dft_color->setMinimumSize(h, h);
        ui_.widget_normal_color_negative->setMinimumSize(h, h);
        ui_.widget_normal_color_positive->setMinimumSize(h, h);
        ui_.widget_wireframe_color->setMinimumSize(h, h);
}

void ColorsWidget::set_view(view::View* const view)
{
        view_ = view;
}

void ColorsWidget::on_background_color_clicked()
{
        QPointer ptr(this);
        dialogs::color_dialog(
                "Background Color", background_color_,
                [&](const QColor& c)
                {
                        if (!ptr.isNull())
                        {
                                ptr->set_background_color(c);
                        }
                });
}

void ColorsWidget::on_wireframe_color_clicked()
{
        QPointer ptr(this);
        dialogs::color_dialog(
                "Wireframe Color", wireframe_color_,
                [&](const QColor& c)
                {
                        if (!ptr.isNull())
                        {
                                set_wireframe_color(c);
                        }
                });
}

void ColorsWidget::on_clip_plane_color_clicked()
{
        QPointer ptr(this);
        dialogs::color_dialog(
                "Clip Plane Color", clip_plane_color_,
                [&](const QColor& c)
                {
                        if (!ptr.isNull())
                        {
                                set_clip_plane_color(c);
                        }
                });
}

void ColorsWidget::on_normal_color_positive_clicked()
{
        QPointer ptr(this);
        dialogs::color_dialog(
                "Positive Normal Color", normal_color_positive_,
                [&](const QColor& c)
                {
                        if (!ptr.isNull())
                        {
                                set_normal_color_positive(c);
                        }
                });
}

void ColorsWidget::on_normal_color_negative_clicked()
{
        QPointer ptr(this);
        dialogs::color_dialog(
                "Negative Normal Color", normal_color_negative_,
                [&](const QColor& c)
                {
                        if (!ptr.isNull())
                        {
                                set_normal_color_negative(c);
                        }
                });
}

void ColorsWidget::on_dft_background_color_clicked()
{
        QPointer ptr(this);
        dialogs::color_dialog(
                "DFT Background Color", dft_background_color_,
                [&](const QColor& c)
                {
                        if (!ptr.isNull())
                        {
                                set_dft_background_color(c);
                        }
                });
}

void ColorsWidget::on_dft_color_clicked()
{
        QPointer ptr(this);
        dialogs::color_dialog(
                "DFT Color", dft_color_,
                [&](const QColor& c)
                {
                        if (!ptr.isNull())
                        {
                                set_dft_color(c);
                        }
                });
}

void ColorsWidget::set_background_color(const QColor& c)
{
        background_color_ = c;
        if (view_)
        {
                view_->send(view::command::SetBackgroundColor(com::qcolor_to_color(c)));
        }
        com::set_widget_color(ui_.widget_background_color, c);
}

void ColorsWidget::set_wireframe_color(const QColor& c)
{
        wireframe_color_ = c;
        if (view_)
        {
                view_->send(view::command::SetWireframeColor(com::qcolor_to_color(c)));
        }
        com::set_widget_color(ui_.widget_wireframe_color, c);
}

void ColorsWidget::set_clip_plane_color(const QColor& c)
{
        clip_plane_color_ = c;
        if (view_)
        {
                view_->send(view::command::SetClipPlaneColor(com::qcolor_to_color(c)));
        }
        com::set_widget_color(ui_.widget_clip_plane_color, c);
}

void ColorsWidget::set_normal_color_positive(const QColor& c)
{
        normal_color_positive_ = c;
        if (view_)
        {
                view_->send(view::command::SetNormalColorPositive(com::qcolor_to_color(c)));
        }
        com::set_widget_color(ui_.widget_normal_color_positive, c);
}

void ColorsWidget::set_normal_color_negative(const QColor& c)
{
        normal_color_negative_ = c;
        if (view_)
        {
                view_->send(view::command::SetNormalColorNegative(com::qcolor_to_color(c)));
        }
        com::set_widget_color(ui_.widget_normal_color_negative, c);
}

void ColorsWidget::set_dft_background_color(const QColor& c)
{
        dft_background_color_ = c;
        if (view_)
        {
                view_->send(view::command::DftSetBackgroundColor(com::qcolor_to_color(c)));
        }
        com::set_widget_color(ui_.widget_dft_background_color, c);
}

void ColorsWidget::set_dft_color(const QColor& c)
{
        dft_color_ = c;
        if (view_)
        {
                view_->send(view::command::DftSetColor(com::qcolor_to_color(c)));
        }
        com::set_widget_color(ui_.widget_dft_color, c);
}

std::vector<view::Command> ColorsWidget::commands() const
{
        return {view::command::SetBackgroundColor(com::qcolor_to_color(background_color_)),
                view::command::SetWireframeColor(com::qcolor_to_color(wireframe_color_)),
                view::command::SetClipPlaneColor(com::qcolor_to_color(clip_plane_color_)),
                view::command::SetNormalColorPositive(com::qcolor_to_color(normal_color_positive_)),
                view::command::SetNormalColorNegative(com::qcolor_to_color(normal_color_negative_)),
                view::command::DftSetBackgroundColor(com::qcolor_to_color(dft_background_color_)),
                view::command::DftSetColor(com::qcolor_to_color(dft_color_))};
}

color::Color ColorsWidget::background_color() const
{
        return com::qcolor_to_color(background_color_);
}
}
