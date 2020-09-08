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

#include "view_widget.h"

#include "../com/support.h"

#include <src/com/error.h>

namespace gui
{
namespace
{
constexpr float NORMAL_LENGTH_MINIMUM = 0.001;
constexpr float NORMAL_LENGTH_DEFAULT = 0.05;
constexpr float NORMAL_LENGTH_MAXIMUM = 0.2;
static_assert(NORMAL_LENGTH_DEFAULT >= NORMAL_LENGTH_MINIMUM);
static_assert(NORMAL_LENGTH_DEFAULT <= NORMAL_LENGTH_MAXIMUM);
static_assert(NORMAL_LENGTH_MAXIMUM - NORMAL_LENGTH_MINIMUM);

// увеличение текстуры тени по сравнению с размером окна.
constexpr int SHADOW_ZOOM = 2;

constexpr double DFT_MAX_BRIGHTNESS = 50000;
constexpr double DFT_GAMMA = 0.5;
}
ViewWidget::ViewWidget(QWidget* parent, std::unique_ptr<view::View>* view) : QWidget(parent), m_view(*view)
{
        ui.setupUi(this);

        ui.checkBox_clip_plane->setChecked(false);
        ui.slider_clip_plane->setEnabled(false);
        set_slider_position(ui.slider_clip_plane, 0.5);
        // Должно быть точное среднее положение
        ASSERT(((ui.slider_clip_plane->maximum() - ui.slider_clip_plane->minimum()) & 1) == 0);

        ui.checkBox_normals->setChecked(false);
        ui.slider_normals->setEnabled(false);
        set_slider_position(
                ui.slider_normals,
                (NORMAL_LENGTH_DEFAULT - NORMAL_LENGTH_MINIMUM) / (NORMAL_LENGTH_MAXIMUM - NORMAL_LENGTH_MINIMUM));

        ui.slider_shadow_quality->setSliderPosition(SHADOW_ZOOM);

        on_dft_clicked();
        on_shadow_clicked();
        on_clip_plane_clicked();

        connect(ui.checkBox_clip_plane, &QCheckBox::clicked, this, &ViewWidget::on_clip_plane_clicked);
        connect(ui.checkBox_convex_hull_2d, &QCheckBox::clicked, this, &ViewWidget::on_convex_hull_2d_clicked);
        connect(ui.checkBox_dft, &QCheckBox::clicked, this, &ViewWidget::on_dft_clicked);
        connect(ui.checkBox_fog, &QCheckBox::clicked, this, &ViewWidget::on_fog_clicked);
        connect(ui.checkBox_fps, &QCheckBox::clicked, this, &ViewWidget::on_fps_clicked);
        connect(ui.checkBox_materials, &QCheckBox::clicked, this, &ViewWidget::on_materials_clicked);
        connect(ui.checkBox_normals, &QCheckBox::clicked, this, &ViewWidget::on_normals_clicked);
        connect(ui.checkBox_optical_flow, &QCheckBox::clicked, this, &ViewWidget::on_optical_flow_clicked);
        connect(ui.checkBox_pencil_sketch, &QCheckBox::clicked, this, &ViewWidget::on_pencil_sketch_clicked);
        connect(ui.checkBox_shadow, &QCheckBox::clicked, this, &ViewWidget::on_shadow_clicked);
        connect(ui.checkBox_smooth, &QCheckBox::clicked, this, &ViewWidget::on_smooth_clicked);
        connect(ui.checkBox_vertical_sync, &QCheckBox::clicked, this, &ViewWidget::on_vertical_sync_clicked);
        connect(ui.checkBox_wireframe, &QCheckBox::clicked, this, &ViewWidget::on_wireframe_clicked);
        connect(ui.pushButton_reset_view, &QPushButton::clicked, this, &ViewWidget::on_reset_view_clicked);
        connect(ui.slider_clip_plane, &QSlider::valueChanged, this, &ViewWidget::on_clip_plane_changed);
        connect(ui.slider_dft_brightness, &QSlider::valueChanged, this, &ViewWidget::on_dft_brightness_changed);
        connect(ui.slider_normals, &QSlider::valueChanged, this, &ViewWidget::on_normals_changed);
        connect(ui.slider_shadow_quality, &QSlider::valueChanged, this, &ViewWidget::on_shadow_quality_changed);
}

void ViewWidget::on_clip_plane_clicked()
{
        constexpr double default_position = 0.5;

        bool checked = ui.checkBox_clip_plane->isChecked();

        ui.slider_clip_plane->setEnabled(checked);
        {
                QSignalBlocker blocker(ui.slider_clip_plane);
                set_slider_position(ui.slider_clip_plane, default_position);
        }

        if (checked)
        {
                if (m_view)
                {
                        m_view->send(view::command::ClipPlaneShow(slider_position(ui.slider_clip_plane)));
                }
        }
        else
        {
                if (m_view)
                {
                        m_view->send(view::command::ClipPlaneHide());
                }
        }
}

void ViewWidget::on_convex_hull_2d_clicked()
{
        m_view->send(view::command::ShowConvexHull2D(ui.checkBox_convex_hull_2d->isChecked()));
}

void ViewWidget::on_dft_clicked()
{
        bool checked = ui.checkBox_dft->isChecked();

        ui.label_dft_brightness->setEnabled(checked);
        ui.slider_dft_brightness->setEnabled(checked);

        if (m_view)
        {
                m_view->send(view::command::ShowDft(checked));
        }
}

void ViewWidget::on_fog_clicked()
{
        m_view->send(view::command::ShowFog(ui.checkBox_fog->isChecked()));
}

void ViewWidget::on_fps_clicked()
{
        m_view->send(view::command::ShowFps(ui.checkBox_fps->isChecked()));
}

void ViewWidget::on_materials_clicked()
{
        m_view->send(view::command::ShowMaterials(ui.checkBox_materials->isChecked()));
}

void ViewWidget::on_normals_clicked()
{
        bool checked = ui.checkBox_normals->isChecked();
        ui.slider_normals->setEnabled(checked);
        m_view->send(view::command::ShowNormals(checked));
}

void ViewWidget::on_optical_flow_clicked()
{
        m_view->send(view::command::ShowOpticalFlow(ui.checkBox_optical_flow->isChecked()));
}

void ViewWidget::on_pencil_sketch_clicked()
{
        m_view->send(view::command::ShowPencilSketch(ui.checkBox_pencil_sketch->isChecked()));
}

void ViewWidget::on_shadow_clicked()
{
        bool checked = ui.checkBox_shadow->isChecked();

        ui.label_shadow_quality->setEnabled(checked);
        ui.slider_shadow_quality->setEnabled(checked);

        if (m_view)
        {
                m_view->send(view::command::ShowShadow(checked));
        }
}

void ViewWidget::on_smooth_clicked()
{
        m_view->send(view::command::ShowSmooth(ui.checkBox_smooth->isChecked()));
}

void ViewWidget::on_vertical_sync_clicked()
{
        m_view->send(view::command::SetVerticalSync(ui.checkBox_vertical_sync->isChecked()));
}

void ViewWidget::on_wireframe_clicked()
{
        m_view->send(view::command::ShowWireframe(ui.checkBox_wireframe->isChecked()));
}

void ViewWidget::on_reset_view_clicked()
{
        m_view->send(view::command::ResetView());
}

void ViewWidget::on_clip_plane_changed(int)
{
        m_view->send(view::command::ClipPlanePosition(slider_position(ui.slider_clip_plane)));
}

void ViewWidget::on_dft_brightness_changed(int)
{
        m_view->send(view::command::SetDftBrightness(dft_brightness()));
}

void ViewWidget::on_normals_changed(int)
{
        m_view->send(view::command::SetNormalLength(normal_length()));
}

void ViewWidget::on_shadow_quality_changed(int)
{
        if (m_view)
        {
                m_view->send(view::command::SetShadowZoom(shadow_zoom()));
        }
}

double ViewWidget::dft_brightness() const
{
        double value = ui.slider_dft_brightness->value() - ui.slider_dft_brightness->minimum();
        double delta = ui.slider_dft_brightness->maximum() - ui.slider_dft_brightness->minimum();
        double value_gamma = std::pow(value / delta, DFT_GAMMA);
        return std::pow(DFT_MAX_BRIGHTNESS, value_gamma);
}

double ViewWidget::shadow_zoom() const
{
        return ui.slider_shadow_quality->value();
}

double ViewWidget::normal_length() const
{
        return interpolation(NORMAL_LENGTH_MINIMUM, NORMAL_LENGTH_MAXIMUM, slider_position(ui.slider_normals));
}

bool ViewWidget::smooth_checked() const
{
        return ui.checkBox_smooth->isChecked();
}

bool ViewWidget::wireframe_checked() const
{
        return ui.checkBox_wireframe->isChecked();
}

bool ViewWidget::shadow_checked() const
{
        return ui.checkBox_shadow->isChecked();
}

bool ViewWidget::fog_checked() const
{
        return ui.checkBox_fog->isChecked();
}

bool ViewWidget::materials_checked() const
{
        return ui.checkBox_materials->isChecked();
}

bool ViewWidget::fps_checked() const
{
        return ui.checkBox_fps->isChecked();
}

bool ViewWidget::pencil_sketch_checked() const
{
        return ui.checkBox_pencil_sketch->isChecked();
}

bool ViewWidget::dft_checked() const
{
        return ui.checkBox_dft->isChecked();
}

bool ViewWidget::convex_hull_2d_checked() const
{
        return ui.checkBox_convex_hull_2d->isChecked();
}

bool ViewWidget::optical_flow_checked() const
{
        return ui.checkBox_optical_flow->isChecked();
}

bool ViewWidget::normals_checked() const
{
        return ui.checkBox_normals->isChecked();
}

bool ViewWidget::vertical_sync_checked() const
{
        return ui.checkBox_vertical_sync->isChecked();
}
}
