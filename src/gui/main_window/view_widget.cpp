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

#include "view_widget.h"

#include <src/com/error.h>
#include <src/com/message.h>
#include <src/gui/com/support.h>
#include <src/view/event.h>
#include <src/view/view.h>

#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QString>
#include <QWidget>

#include <cmath>
#include <optional>
#include <vector>

namespace ns::gui::main_window
{
namespace
{
constexpr double NORMAL_LENGTH_MINIMUM = 0.001;
constexpr double NORMAL_LENGTH_DEFAULT = 0.05;
constexpr double NORMAL_LENGTH_MAXIMUM = 0.2;
static_assert(NORMAL_LENGTH_DEFAULT >= NORMAL_LENGTH_MINIMUM);
static_assert(NORMAL_LENGTH_DEFAULT <= NORMAL_LENGTH_MAXIMUM);
static_assert(NORMAL_LENGTH_MAXIMUM - NORMAL_LENGTH_MINIMUM > 0);

constexpr int SHADOW_ZOOM = 2;

constexpr double DFT_MAX_BRIGHTNESS = 50000;
constexpr double DFT_GAMMA = 0.5;
}

ViewWidget::ViewWidget()
        : QWidget(nullptr)
{
        ui_.setupUi(this);

        ui_.label_shadow_quality->setVisible(false);
        ui_.slider_shadow_quality->setVisible(false);

        ui_.check_box_clip_plane->setChecked(false);
        ui_.check_box_clip_plane_lines->setEnabled(false);
        ui_.check_box_clip_plane_lines->setChecked(true);
        ui_.slider_clip_plane->setEnabled(false);
        set_slider_position(ui_.slider_clip_plane, 0.5);
        ASSERT(((ui_.slider_clip_plane->maximum() - ui_.slider_clip_plane->minimum()) & 1) == 0);

        ui_.check_box_normals->setChecked(false);
        ui_.slider_normals->setEnabled(false);
        set_slider_position(
                ui_.slider_normals,
                (NORMAL_LENGTH_DEFAULT - NORMAL_LENGTH_MINIMUM) / (NORMAL_LENGTH_MAXIMUM - NORMAL_LENGTH_MINIMUM));

        ui_.slider_shadow_quality->setSliderPosition(SHADOW_ZOOM);

        on_dft_clicked();
        on_shadow_clicked();
        on_clip_plane_clicked();

        connect(ui_.check_box_clip_plane, &QCheckBox::clicked, this, &ViewWidget::on_clip_plane_clicked);
        connect(ui_.check_box_clip_plane_lines, &QCheckBox::clicked, this, &ViewWidget::on_clip_plane_lines_clicked);
        connect(ui_.check_box_convex_hull_2d, &QCheckBox::clicked, this, &ViewWidget::on_convex_hull_2d_clicked);
        connect(ui_.check_box_dft, &QCheckBox::clicked, this, &ViewWidget::on_dft_clicked);
        connect(ui_.check_box_fog, &QCheckBox::clicked, this, &ViewWidget::on_fog_clicked);
        connect(ui_.check_box_fps, &QCheckBox::clicked, this, &ViewWidget::on_fps_clicked);
        connect(ui_.check_box_materials, &QCheckBox::clicked, this, &ViewWidget::on_materials_clicked);
        connect(ui_.check_box_normals, &QCheckBox::clicked, this, &ViewWidget::on_normals_clicked);
        connect(ui_.check_box_optical_flow, &QCheckBox::clicked, this, &ViewWidget::on_optical_flow_clicked);
        connect(ui_.check_box_pencil_sketch, &QCheckBox::clicked, this, &ViewWidget::on_pencil_sketch_clicked);
        connect(ui_.check_box_shadow, &QCheckBox::clicked, this, &ViewWidget::on_shadow_clicked);
        connect(ui_.check_box_flat_shading, &QCheckBox::clicked, this, &ViewWidget::on_flat_shading_clicked);
        connect(ui_.check_box_vertical_sync, &QCheckBox::clicked, this, &ViewWidget::on_vertical_sync_clicked);
        connect(ui_.check_box_wireframe, &QCheckBox::clicked, this, &ViewWidget::on_wireframe_clicked);
        connect(ui_.push_button_reset_view, &QPushButton::clicked, this, &ViewWidget::on_reset_view_clicked);
        connect(ui_.slider_clip_plane, &QSlider::valueChanged, this, &ViewWidget::on_clip_plane_changed);
        connect(ui_.slider_dft_brightness, &QSlider::valueChanged, this, &ViewWidget::on_dft_brightness_changed);
        connect(ui_.slider_normals, &QSlider::valueChanged, this, &ViewWidget::on_normals_changed);
        connect(ui_.slider_shadow_quality, &QSlider::valueChanged, this, &ViewWidget::on_shadow_quality_changed);
}

void ViewWidget::set_view(view::View* const view)
{
        view_ = view;

        std::optional<view::info::Functionality> functionality;
        std::optional<view::info::SampleCount> sample_count;
        view_->receive({&functionality, &sample_count});
        if (!functionality || !sample_count)
        {
                message_error_fatal("Failed to receive view information");
                return;
        }

        set_functionality(*functionality);
        set_sample_count(*sample_count);
}

void ViewWidget::set_functionality(const view::info::Functionality& functionality)
{
        ui_.label_shadow_quality->setVisible(functionality.shadow_zoom);
        ui_.slider_shadow_quality->setVisible(functionality.shadow_zoom);
}

void ViewWidget::set_sample_count(const view::info::SampleCount& sample_count)
{
        const auto name = [](const int count)
        {
                if (count != 1)
                {
                        return QString("%1 samples").arg(count);
                }
                return QString("1 sample");
        };

        for (const int count : sample_count.sample_counts)
        {
                auto* const button = new QRadioButton(this);
                button->setText(name(count));
                button->setChecked(count == sample_count.sample_count);
                ui_.vertical_layout_sample_counts->addWidget(button);
                connect(button, &QRadioButton::toggled, this,
                        [this, count](const bool checked)
                        {
                                if (checked)
                                {
                                        view_->send(view::command::SetSampleCount(count));
                                }
                        });
        }
}

void ViewWidget::on_clip_plane_clicked()
{
        constexpr double DEFAULT_POSITION = 0.5;

        const bool checked = ui_.check_box_clip_plane->isChecked();

        ui_.check_box_clip_plane_lines->setEnabled(checked);

        ui_.slider_clip_plane->setEnabled(checked);
        {
                const QSignalBlocker blocker(ui_.slider_clip_plane);
                set_slider_position(ui_.slider_clip_plane, DEFAULT_POSITION);
        }

        if (view_)
        {
                if (checked)
                {
                        view_->send(view::command::ClipPlaneShow(slider_position(ui_.slider_clip_plane)));
                }
                else
                {
                        view_->send(view::command::ClipPlaneHide());
                }
        }
}

void ViewWidget::on_clip_plane_lines_clicked()
{
        view_->send(view::command::ShowClipPlaneLines(ui_.check_box_clip_plane_lines->isChecked()));
}

void ViewWidget::on_convex_hull_2d_clicked()
{
        view_->send(view::command::ConvexHullShow(ui_.check_box_convex_hull_2d->isChecked()));
}

void ViewWidget::on_dft_clicked()
{
        const bool checked = ui_.check_box_dft->isChecked();

        ui_.label_dft_brightness->setEnabled(checked);
        ui_.slider_dft_brightness->setEnabled(checked);

        if (view_)
        {
                view_->send(view::command::DftShow(checked));
        }
}

void ViewWidget::on_fog_clicked()
{
        view_->send(view::command::ShowFog(ui_.check_box_fog->isChecked()));
}

void ViewWidget::on_fps_clicked()
{
        view_->send(view::command::ShowFps(ui_.check_box_fps->isChecked()));
}

void ViewWidget::on_materials_clicked()
{
        view_->send(view::command::ShowMaterials(ui_.check_box_materials->isChecked()));
}

void ViewWidget::on_normals_clicked()
{
        const bool checked = ui_.check_box_normals->isChecked();
        ui_.slider_normals->setEnabled(checked);
        view_->send(view::command::ShowNormals(checked));
}

void ViewWidget::on_optical_flow_clicked()
{
        view_->send(view::command::OpticalFlowShow(ui_.check_box_optical_flow->isChecked()));
}

void ViewWidget::on_pencil_sketch_clicked()
{
        view_->send(view::command::PencilSketchShow(ui_.check_box_pencil_sketch->isChecked()));
}

void ViewWidget::on_shadow_clicked()
{
        const bool checked = ui_.check_box_shadow->isChecked();

        ui_.label_shadow_quality->setEnabled(checked);
        ui_.slider_shadow_quality->setEnabled(checked);

        if (view_)
        {
                view_->send(view::command::ShowShadow(checked));
        }
}

void ViewWidget::on_flat_shading_clicked()
{
        view_->send(view::command::SetFlatShading(ui_.check_box_flat_shading->isChecked()));
}

void ViewWidget::on_vertical_sync_clicked()
{
        view_->send(view::command::SetVerticalSync(ui_.check_box_vertical_sync->isChecked()));
}

void ViewWidget::on_wireframe_clicked()
{
        view_->send(view::command::ShowWireframe(ui_.check_box_wireframe->isChecked()));
}

void ViewWidget::on_reset_view_clicked()
{
        view_->send(view::command::ResetView());
}

void ViewWidget::on_clip_plane_changed(int)
{
        view_->send(view::command::ClipPlaneSetPosition(slider_position(ui_.slider_clip_plane)));
}

void ViewWidget::on_dft_brightness_changed(int)
{
        view_->send(view::command::DftSetBrightness(dft_brightness()));
}

void ViewWidget::on_normals_changed(int)
{
        view_->send(view::command::SetNormalLength(normal_length()));
}

void ViewWidget::on_shadow_quality_changed(int)
{
        if (view_)
        {
                view_->send(view::command::SetShadowZoom(shadow_zoom()));
        }
}

std::vector<view::Command> ViewWidget::commands() const
{
        return {view::command::ConvexHullShow(ui_.check_box_convex_hull_2d->isChecked()),
                view::command::DftSetBrightness(dft_brightness()),
                view::command::DftShow(ui_.check_box_dft->isChecked()),
                view::command::OpticalFlowShow(ui_.check_box_optical_flow->isChecked()),
                view::command::PencilSketchShow(ui_.check_box_pencil_sketch->isChecked()),
                view::command::SetFlatShading(ui_.check_box_flat_shading->isChecked()),
                view::command::SetNormalLength(normal_length()),
                view::command::SetShadowZoom(shadow_zoom()),
                view::command::SetVerticalSync(ui_.check_box_vertical_sync->isChecked()),
                view::command::ShowClipPlaneLines(ui_.check_box_clip_plane_lines->isChecked()),
                view::command::ShowFog(ui_.check_box_fog->isChecked()),
                view::command::ShowFps(ui_.check_box_fps->isChecked()),
                view::command::ShowMaterials(ui_.check_box_materials->isChecked()),
                view::command::ShowNormals(ui_.check_box_normals->isChecked()),
                view::command::ShowShadow(ui_.check_box_shadow->isChecked()),
                view::command::ShowWireframe(ui_.check_box_wireframe->isChecked())};
}

double ViewWidget::dft_brightness() const
{
        const double value = ui_.slider_dft_brightness->value() - ui_.slider_dft_brightness->minimum();
        const double delta = ui_.slider_dft_brightness->maximum() - ui_.slider_dft_brightness->minimum();
        const double value_gamma = std::pow(value / delta, DFT_GAMMA);
        return std::pow(DFT_MAX_BRIGHTNESS, value_gamma);
}

double ViewWidget::shadow_zoom() const
{
        return ui_.slider_shadow_quality->value();
}

double ViewWidget::normal_length() const
{
        return std::lerp(NORMAL_LENGTH_MINIMUM, NORMAL_LENGTH_MAXIMUM, slider_position(ui_.slider_normals));
}
}
