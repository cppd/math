/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "painter_parameters_3d.h"

#include "message.h"
#include "painter_parameters.h"

#include <src/com/error.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/gui/com/support.h>

#include <QDialog>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QString>
#include <QtGlobal>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <string>
#include <tuple>

namespace ns::gui::dialogs
{
namespace
{
constexpr int DIMENSION = 3;

namespace
{
void check_parameters(const int width, const int height, const int max_screen_size)
{
        if (!(width >= 1 && height >= 1))
        {
                error("Width " + to_string(width) + " and height " + to_string(height)
                      + " must be greater than or equal to 1");
        }

        if (!(max_screen_size >= 1))
        {
                error("Maximum screen size " + to_string(max_screen_size) + " must be greater than or equal to 1");
        }
}
}
}

PainterParameters3dDialog::PainterParameters3dDialog(
        const int max_thread_count,
        const int width,
        const int height,
        const int max_screen_size,
        const int samples_per_pixel,
        const int max_samples_per_pixel,
        const std::array<const char*, 2>& precisions,
        const int precision_index,
        const std::array<const char*, 2>& colors,
        const int color_index,
        const std::array<const char*, 2>& integrators,
        const int integrator_index,
        std::optional<std::tuple<PainterParameters, PainterParameters3d>>* const parameters)
        : QDialog(com::parent_for_dialog()),
          parameters_widget_(new PainterParametersWidget(
                  this,
                  max_thread_count,
                  samples_per_pixel,
                  max_samples_per_pixel,
                  precisions,
                  precision_index,
                  colors,
                  color_index,
                  integrators,
                  integrator_index)),
          aspect_ratio_(static_cast<double>(width) / height),
          max_width_(aspect_ratio_ >= 1 ? max_screen_size : std::lround(max_screen_size * aspect_ratio_)),
          min_width_(std::min(max_width_, width)),
          max_height_(aspect_ratio_ >= 1 ? std::lround(max_screen_size / aspect_ratio_) : max_screen_size),
          min_height_(std::min(max_height_, height)),
          parameters_(parameters)
{
        ui_.setupUi(this);
        setWindowTitle("Painter");

        connect(ui_.spin_box_width, QOverload<int>::of(&QSpinBox::valueChanged), this,
                &PainterParameters3dDialog::on_width_value_changed);
        connect(ui_.spin_box_height, QOverload<int>::of(&QSpinBox::valueChanged), this,
                &PainterParameters3dDialog::on_height_value_changed);

        ui_.label_space->setText(QString::fromStdString(space_name(DIMENSION)));

        ui_.spin_box_width->setMinimum(min_width_);
        ui_.spin_box_width->setMaximum(max_width_);
        ui_.spin_box_width->setValue(min_width_);
        ui_.spin_box_width->setSingleStep(std::max(1, min_width_ / 10));

        ui_.spin_box_height->setMinimum(min_height_);
        ui_.spin_box_height->setMaximum(max_height_);
        ui_.spin_box_height->setValue(min_height_);
        ui_.spin_box_height->setSingleStep(std::max(1, min_height_ / 10));

        ui_.vertical_layout_parameters->addWidget(parameters_widget_);

        com::set_dialog_size(this);
}

void PainterParameters3dDialog::on_width_value_changed(int)
{
        const int height = std::lround(ui_.spin_box_width->value() / aspect_ratio_);

        const QSignalBlocker blocker(ui_.spin_box_height);
        ui_.spin_box_height->setValue(std::clamp(height, min_height_, max_height_));
}

void PainterParameters3dDialog::on_height_value_changed(int)
{
        const int width = std::lround(ui_.spin_box_height->value() * aspect_ratio_);

        const QSignalBlocker blocker(ui_.spin_box_width);
        ui_.spin_box_width->setValue(std::clamp(width, min_width_, max_width_));
}

void PainterParameters3dDialog::done(const int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        if (!parameters_widget_->check())
        {
                return;
        }

        const int width = ui_.spin_box_width->value();
        if (!(min_width_ <= width && width <= max_width_))
        {
                const std::string msg =
                        "Width must be in the range [" + to_string(min_width_) + ", " + to_string(max_width_) + "]";
                dialogs::message_critical(msg);
                return;
        }

        const int height = ui_.spin_box_height->value();
        if (!(min_height_ <= height && height <= max_height_))
        {
                const std::string msg =
                        "Height must be in the range [" + to_string(min_height_) + ", " + to_string(max_height_) + "]";
                dialogs::message_critical(msg);
                return;
        }

        auto& parameters = parameters_->emplace();
        std::get<0>(parameters) = parameters_widget_->parameters();
        std::get<1>(parameters).width = width;
        std::get<1>(parameters).height = height;

        QDialog::done(r);
}

std::optional<std::tuple<PainterParameters, PainterParameters3d>> PainterParameters3dDialog::show(
        const int max_thread_count,
        const int width,
        const int height,
        const int max_screen_size,
        const int samples_per_pixel,
        const int max_samples_per_pixel,
        const std::array<const char*, 2>& precisions,
        const int precision_index,
        const std::array<const char*, 2>& colors,
        const int color_index,
        const std::array<const char*, 2>& integrators,
        const int integrator_index)
{
        check_parameters(width, height, max_screen_size);

        std::optional<std::tuple<PainterParameters, PainterParameters3d>> parameters;

        const com::QtObjectInDynamicMemory w(new PainterParameters3dDialog(
                max_thread_count, width, height, max_screen_size, samples_per_pixel, max_samples_per_pixel, precisions,
                precision_index, colors, color_index, integrators, integrator_index, &parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }

        ASSERT(parameters);
        return parameters;
}
}
