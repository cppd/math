/*
Copyright (C) 2017-2020 Topological Manifold

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
#include "../dialogs/message.h"

#include <src/com/error.h>

namespace gui
{
namespace
{
constexpr QRgb BACKGROUND_COLOR = qRgb(50, 100, 150);
constexpr QRgb SPECULAR_COLOR = qRgb(255, 255, 255);
constexpr QRgb WIREFRAME_COLOR = qRgb(255, 255, 255);
constexpr QRgb CLIP_PLANE_COLOR = qRgb(250, 230, 150);
constexpr QRgb NORMAL_COLOR_POSITIVE = qRgb(200, 200, 0);
constexpr QRgb NORMAL_COLOR_NEGATIVE = qRgb(50, 150, 50);
constexpr QRgb DFT_BACKGROUND_COLOR = qRgb(0, 0, 50);
constexpr QRgb DFT_COLOR = qRgb(150, 200, 250);

constexpr double MAXIMUM_LIGHTING_INTENSITY = 3.0;
}

ColorsWidget::ColorsWidget(QWidget* parent, std::unique_ptr<view::View>* view) : QWidget(parent), m_view(*view)
{
        ui.setupUi(this);

        // Должно быть точное среднее положение
        ASSERT(((ui.slider_lighting_intensity->maximum() - ui.slider_lighting_intensity->minimum()) & 1) == 0);
        set_slider_to_middle(ui.slider_lighting_intensity);

        set_background_color(BACKGROUND_COLOR);
        set_specular_color(SPECULAR_COLOR);
        set_wireframe_color(WIREFRAME_COLOR);
        set_clip_plane_color(CLIP_PLANE_COLOR);
        set_normal_color_positive(NORMAL_COLOR_POSITIVE);
        set_normal_color_negative(NORMAL_COLOR_NEGATIVE);
        set_dft_background_color(DFT_BACKGROUND_COLOR);
        set_dft_color(DFT_COLOR);

        connect(ui.pushButton_reset_lighting, &QPushButton::clicked, this, &ColorsWidget::on_reset_lighting_clicked);
        connect(ui.slider_lighting_intensity, &QSlider::valueChanged, this,
                &ColorsWidget::on_lighting_intensity_changed);

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

void ColorsWidget::on_reset_lighting_clicked()
{
        bool yes;
        if (!dialog::message_question_default_yes("Reset lighting?", &yes) || !yes)
        {
                return;
        }

        set_slider_to_middle(ui.slider_lighting_intensity);
}

void ColorsWidget::on_lighting_intensity_changed(int)
{
        m_view->send(view::command::SetLightingIntensity(lighting_intensity()));
}

void ColorsWidget::on_background_color_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("Background Color", m_background_color, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        ptr->set_background_color(c);
                }
        });
}

void ColorsWidget::on_wireframe_color_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("Wireframe Color", m_wireframe_color, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        set_wireframe_color(c);
                }
        });
}

void ColorsWidget::on_clip_plane_color_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("Clip Plane Color", m_clip_plane_color, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        set_clip_plane_color(c);
                }
        });
}

void ColorsWidget::on_normal_color_positive_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("Positive Normal Color", m_normal_color_positive, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        set_normal_color_positive(c);
                }
        });
}

void ColorsWidget::on_normal_color_negative_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("Negative Normal Color", m_normal_color_negative, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        set_normal_color_negative(c);
                }
        });
}

void ColorsWidget::on_dft_background_color_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("DFT Background Color", m_dft_background_color, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        set_dft_background_color(c);
                }
        });
}

void ColorsWidget::on_dft_color_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog("DFT Color", m_dft_color, [&](const QColor& c) {
                if (!ptr.isNull())
                {
                        set_dft_color(c);
                }
        });
}

void ColorsWidget::set_background_color(const QColor& c)
{
        m_background_color = c;
        if (m_view)
        {
                m_view->send(view::command::SetBackgroundColor(qcolor_to_rgb(c)));
        }
        set_widget_color(ui.widget_background_color, c);
}

void ColorsWidget::set_specular_color(const QColor& c)
{
        m_specular_color = c;
        if (m_view)
        {
                m_view->send(view::command::SetSpecularColor(qcolor_to_rgb(c)));
        }
}

void ColorsWidget::set_wireframe_color(const QColor& c)
{
        m_wireframe_color = c;
        if (m_view)
        {
                m_view->send(view::command::SetWireframeColor(qcolor_to_rgb(c)));
        }
        set_widget_color(ui.widget_wireframe_color, c);
}

void ColorsWidget::set_clip_plane_color(const QColor& c)
{
        m_clip_plane_color = c;
        if (m_view)
        {
                m_view->send(view::command::SetClipPlaneColor(qcolor_to_rgb(c)));
        }
        set_widget_color(ui.widget_clip_plane_color, c);
}

void ColorsWidget::set_normal_color_positive(const QColor& c)
{
        m_normal_color_positive = c;
        if (m_view)
        {
                m_view->send(view::command::SetNormalColorPositive(qcolor_to_rgb(c)));
        }
        set_widget_color(ui.widget_normal_color_positive, c);
}

void ColorsWidget::set_normal_color_negative(const QColor& c)
{
        m_normal_color_negative = c;
        if (m_view)
        {
                m_view->send(view::command::SetNormalColorNegative(qcolor_to_rgb(c)));
        }
        set_widget_color(ui.widget_normal_color_negative, c);
}

void ColorsWidget::set_dft_background_color(const QColor& c)
{
        m_dft_background_color = c;
        if (m_view)
        {
                m_view->send(view::command::SetDftBackgroundColor(qcolor_to_rgb(c)));
        }
        set_widget_color(ui.widget_dft_background_color, c);
}

void ColorsWidget::set_dft_color(const QColor& c)
{
        m_dft_color = c;
        if (m_view)
        {
                m_view->send(view::command::SetDftColor(qcolor_to_rgb(c)));
        }
        set_widget_color(ui.widget_dft_color, c);
}

Color ColorsWidget::background_color() const
{
        return qcolor_to_rgb(m_background_color);
}

Color ColorsWidget::specular_color() const
{
        return qcolor_to_rgb(m_specular_color);
}

Color ColorsWidget::wireframe_color() const
{
        return qcolor_to_rgb(m_wireframe_color);
}

Color ColorsWidget::clip_plane_color() const
{
        return qcolor_to_rgb(m_clip_plane_color);
}

Color ColorsWidget::normal_color_positive() const
{
        return qcolor_to_rgb(m_normal_color_positive);
}

Color ColorsWidget::normal_color_negative() const
{
        return qcolor_to_rgb(m_normal_color_negative);
}

Color ColorsWidget::dft_background_color() const
{
        return qcolor_to_rgb(m_dft_background_color);
}

Color ColorsWidget::dft_color() const
{
        return qcolor_to_rgb(m_dft_color);
}

double ColorsWidget::lighting_intensity() const
{
        double v = 2.0 * slider_position(ui.slider_lighting_intensity);
        return (v <= 1.0) ? v : interpolation(1.0, MAXIMUM_LIGHTING_INTENSITY, v - 1.0);
}
}
