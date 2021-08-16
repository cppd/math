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

#include "colors_widget.h"

#include "../com/support.h"
#include "../dialogs/color_dialog.h"

namespace ns::gui::main_window
{
namespace
{
constexpr QRgb BACKGROUND_COLOR = qRgb(75, 90, 105);
constexpr QRgb WIREFRAME_COLOR = qRgb(255, 255, 255);
constexpr QRgb CLIP_PLANE_COLOR = qRgb(250, 230, 150);
constexpr QRgb NORMAL_COLOR_POSITIVE = qRgb(200, 200, 0);
constexpr QRgb NORMAL_COLOR_NEGATIVE = qRgb(50, 150, 50);
constexpr QRgb DFT_BACKGROUND_COLOR = qRgb(0, 0, 50);
constexpr QRgb DFT_COLOR = qRgb(150, 200, 250);
}

ColorsWidget::ColorsWidget() : QWidget(nullptr)
{
        ui.setupUi(this);

        set_background_color(BACKGROUND_COLOR);
        set_wireframe_color(WIREFRAME_COLOR);
        set_clip_plane_color(CLIP_PLANE_COLOR);
        set_normal_color_positive(NORMAL_COLOR_POSITIVE);
        set_normal_color_negative(NORMAL_COLOR_NEGATIVE);
        set_dft_background_color(DFT_BACKGROUND_COLOR);
        set_dft_color(DFT_COLOR);

        connect(ui.toolButton_background_color, &QToolButton::clicked, this,
                &ColorsWidget::on_background_color_clicked);
        connect(ui.toolButton_clip_plane_color, &QToolButton::clicked, this,
                &ColorsWidget::on_clip_plane_color_clicked);
        connect(ui.toolButton_dft_background_color, &QToolButton::clicked, this,
                &ColorsWidget::on_dft_background_color_clicked);
        connect(ui.toolButton_dft_color, &QToolButton::clicked, this, &ColorsWidget::on_dft_color_clicked);
        connect(ui.toolButton_normal_color_negative, &QToolButton::clicked, this,
                &ColorsWidget::on_normal_color_negative_clicked);
        connect(ui.toolButton_normal_color_positive, &QToolButton::clicked, this,
                &ColorsWidget::on_normal_color_positive_clicked);
        connect(ui.toolButton_wireframe_color, &QToolButton::clicked, this, &ColorsWidget::on_wireframe_color_clicked);
}

void ColorsWidget::set_view(view::View* view)
{
        view_ = view;
}

void ColorsWidget::on_background_color_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog(
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
        dialog::color_dialog(
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
        dialog::color_dialog(
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
        dialog::color_dialog(
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
        dialog::color_dialog(
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
        dialog::color_dialog(
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
        dialog::color_dialog(
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
                view_->send(view::command::SetBackgroundColor(qcolor_to_color(c)));
        }
        set_widget_color(ui.widget_background_color, c);
}

void ColorsWidget::set_wireframe_color(const QColor& c)
{
        wireframe_color_ = c;
        if (view_)
        {
                view_->send(view::command::SetWireframeColor(qcolor_to_color(c)));
        }
        set_widget_color(ui.widget_wireframe_color, c);
}

void ColorsWidget::set_clip_plane_color(const QColor& c)
{
        clip_plane_color_ = c;
        if (view_)
        {
                view_->send(view::command::SetClipPlaneColor(qcolor_to_color(c)));
        }
        set_widget_color(ui.widget_clip_plane_color, c);
}

void ColorsWidget::set_normal_color_positive(const QColor& c)
{
        normal_color_positive_ = c;
        if (view_)
        {
                view_->send(view::command::SetNormalColorPositive(qcolor_to_color(c)));
        }
        set_widget_color(ui.widget_normal_color_positive, c);
}

void ColorsWidget::set_normal_color_negative(const QColor& c)
{
        normal_color_negative_ = c;
        if (view_)
        {
                view_->send(view::command::SetNormalColorNegative(qcolor_to_color(c)));
        }
        set_widget_color(ui.widget_normal_color_negative, c);
}

void ColorsWidget::set_dft_background_color(const QColor& c)
{
        dft_background_color_ = c;
        if (view_)
        {
                view_->send(view::command::SetDftBackgroundColor(qcolor_to_color(c)));
        }
        set_widget_color(ui.widget_dft_background_color, c);
}

void ColorsWidget::set_dft_color(const QColor& c)
{
        dft_color_ = c;
        if (view_)
        {
                view_->send(view::command::SetDftColor(qcolor_to_color(c)));
        }
        set_widget_color(ui.widget_dft_color, c);
}

color::Color ColorsWidget::background_color() const
{
        return qcolor_to_color(background_color_);
}

color::Color ColorsWidget::wireframe_color() const
{
        return qcolor_to_color(wireframe_color_);
}

color::Color ColorsWidget::clip_plane_color() const
{
        return qcolor_to_color(clip_plane_color_);
}

color::Color ColorsWidget::normal_color_positive() const
{
        return qcolor_to_color(normal_color_positive_);
}

color::Color ColorsWidget::normal_color_negative() const
{
        return qcolor_to_color(normal_color_negative_);
}

color::Color ColorsWidget::dft_background_color() const
{
        return qcolor_to_color(dft_background_color_);
}

color::Color ColorsWidget::dft_color() const
{
        return qcolor_to_color(dft_color_);
}
}
