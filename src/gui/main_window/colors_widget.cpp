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
#include "../dialogs/message.h"

#include <sstream>

namespace ns::gui::main_window
{
namespace
{
constexpr QRgb BACKGROUND_COLOR = qRgb(50, 100, 150);
constexpr QRgb WIREFRAME_COLOR = qRgb(255, 255, 255);
constexpr QRgb CLIP_PLANE_COLOR = qRgb(250, 230, 150);
constexpr QRgb NORMAL_COLOR_POSITIVE = qRgb(200, 200, 0);
constexpr QRgb NORMAL_COLOR_NEGATIVE = qRgb(50, 150, 50);
constexpr QRgb DFT_BACKGROUND_COLOR = qRgb(0, 0, 50);
constexpr QRgb DFT_COLOR = qRgb(150, 200, 250);

constexpr double DEFAULT_LIGHTING_INTENSITY = 2.0;
constexpr double MAXIMUM_LIGHTING_INTENSITY = 20.0;
static_assert(MAXIMUM_LIGHTING_INTENSITY > 1);
}

ColorsWidget::ColorsWidget() : QWidget(nullptr)
{
        ui.setupUi(this);

        ui.slider_lighting_intensity->setMinimum(0);
        ui.slider_lighting_intensity->setMaximum(1000);

        set_lighting_intensity(DEFAULT_LIGHTING_INTENSITY, true /*set_slider*/);

        set_background_color(BACKGROUND_COLOR);
        set_wireframe_color(WIREFRAME_COLOR);
        set_clip_plane_color(CLIP_PLANE_COLOR);
        set_normal_color_positive(NORMAL_COLOR_POSITIVE);
        set_normal_color_negative(NORMAL_COLOR_NEGATIVE);
        set_dft_background_color(DFT_BACKGROUND_COLOR);
        set_dft_color(DFT_COLOR);

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

void ColorsWidget::set_view(view::View* view)
{
        m_view = view;
}

void ColorsWidget::set_lighting_intensity(double intensity, bool set_slider)
{
        intensity = std::clamp(intensity, 1 / MAXIMUM_LIGHTING_INTENSITY, MAXIMUM_LIGHTING_INTENSITY);

        if (set_slider)
        {
                double position = std::log(intensity) / std::log(MAXIMUM_LIGHTING_INTENSITY);
                position = (position + 1) / 2;
                QSignalBlocker blocker(ui.slider_lighting_intensity);
                set_slider_position(ui.slider_lighting_intensity, position);

                intensity = lighting_intensity();
        }

        if (m_view)
        {
                m_view->send(view::command::SetLightingIntensity(intensity));
        }

        std::ostringstream oss;
        oss << std::setprecision(2) << std::fixed << intensity;
        ui.label_lighting_intensity->setText(QString::fromStdString(oss.str()));
}

void ColorsWidget::on_lighting_intensity_changed(int)
{
        set_lighting_intensity(lighting_intensity(), false /*set_slider*/);
}

void ColorsWidget::on_background_color_clicked()
{
        QPointer ptr(this);
        dialog::color_dialog(
                "Background Color", m_background_color,
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
                "Wireframe Color", m_wireframe_color,
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
                "Clip Plane Color", m_clip_plane_color,
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
                "Positive Normal Color", m_normal_color_positive,
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
                "Negative Normal Color", m_normal_color_negative,
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
                "DFT Background Color", m_dft_background_color,
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
                "DFT Color", m_dft_color,
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
        m_background_color = c;
        if (m_view)
        {
                m_view->send(view::command::SetBackgroundColor(qcolor_to_rgb(c)));
        }
        set_widget_color(ui.widget_background_color, c);
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
        double v = 2.0 * slider_position(ui.slider_lighting_intensity) - 1;
        return std::pow(MAXIMUM_LIGHTING_INTENSITY, v);
}
}
