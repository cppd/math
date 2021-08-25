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

#include "painter_parameters_3d.h"

#include "message.h"

#include "../com/support.h"

#include <src/com/error.h>
#include <src/com/names.h>
#include <src/com/print.h>

namespace ns::gui::dialog
{
namespace
{
constexpr int DIMENSION = 3;
}

PainterParameters3dDialog::PainterParameters3dDialog(
        int max_thread_count,
        int width,
        int height,
        int max_screen_size,
        int default_samples_per_pixel,
        int max_samples_per_pixel,
        const std::array<const char*, 2>& precisions,
        int default_precision_index,
        const std::array<const char*, 2>& colors,
        int default_color_index,
        std::optional<std::tuple<PainterParameters, PainterParameters3d>>& parameters)
        : QDialog(parent_for_dialog()),
          parameters_widget_(new PainterParametersWidget(
                  this,
                  max_thread_count,
                  default_samples_per_pixel,
                  max_samples_per_pixel,
                  precisions,
                  default_precision_index,
                  colors,
                  default_color_index)),
          parameters_(parameters)
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

        ui_.setupUi(this);
        setWindowTitle("Painter");

        connect(ui_.spinBox_width, QOverload<int>::of(&QSpinBox::valueChanged), this,
                &PainterParameters3dDialog::on_width_value_changed);
        connect(ui_.spinBox_height, QOverload<int>::of(&QSpinBox::valueChanged), this,
                &PainterParameters3dDialog::on_height_value_changed);

        aspect_ratio_ = static_cast<double>(width) / height;
        max_width_ = aspect_ratio_ >= 1 ? max_screen_size : std::lround(max_screen_size * aspect_ratio_);
        max_height_ = aspect_ratio_ >= 1 ? std::lround(max_screen_size / aspect_ratio_) : max_screen_size;
        min_width_ = std::min(max_width_, width);
        min_height_ = std::min(max_height_, height);

        ui_.label_space->setText(QString::fromStdString(space_name(DIMENSION)));

        ui_.spinBox_width->setMinimum(min_width_);
        ui_.spinBox_width->setMaximum(max_width_);
        ui_.spinBox_width->setValue(min_width_);
        ui_.spinBox_width->setSingleStep(std::max(1, min_width_ / 10));

        ui_.spinBox_height->setMinimum(min_height_);
        ui_.spinBox_height->setMaximum(max_height_);
        ui_.spinBox_height->setValue(min_height_);
        ui_.spinBox_height->setSingleStep(std::max(1, min_height_ / 10));

        ui_.verticalLayout_parameters->addWidget(parameters_widget_);
}

void PainterParameters3dDialog::on_width_value_changed(int)
{
        int height = std::lround(ui_.spinBox_width->value() / aspect_ratio_);

        QSignalBlocker blocker(ui_.spinBox_height);
        ui_.spinBox_height->setValue(std::clamp(height, min_height_, max_height_));
}

void PainterParameters3dDialog::on_height_value_changed(int)
{
        int width = std::lround(ui_.spinBox_height->value() * aspect_ratio_);

        QSignalBlocker blocker(ui_.spinBox_width);
        ui_.spinBox_width->setValue(std::clamp(width, min_width_, max_width_));
}

void PainterParameters3dDialog::done(int r)
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

        int width = ui_.spinBox_width->value();
        if (!(min_width_ <= width && width <= max_width_))
        {
                std::string msg =
                        "Width must be in the range [" + to_string(min_width_) + ", " + to_string(max_width_) + "]";
                dialog::message_critical(msg);
                return;
        }

        int height = ui_.spinBox_height->value();
        if (!(min_height_ <= height && height <= max_height_))
        {
                std::string msg =
                        "Height must be in the range [" + to_string(min_height_) + ", " + to_string(max_height_) + "]";
                dialog::message_critical(msg);
                return;
        }

        parameters_.emplace();

        std::get<0>(*parameters_) = parameters_widget_->parameters();
        std::get<1>(*parameters_).width = width;
        std::get<1>(*parameters_).height = height;

        QDialog::done(r);
}

std::optional<std::tuple<PainterParameters, PainterParameters3d>> PainterParameters3dDialog::show(
        int max_thread_count,
        int width,
        int height,
        int max_screen_size,
        int default_samples_per_pixel,
        int max_samples_per_pixel,
        const std::array<const char*, 2>& precisions,
        int default_precision_index,
        const std::array<const char*, 2>& colors,
        int default_color_index)
{
        std::optional<std::tuple<PainterParameters, PainterParameters3d>> parameters;

        QtObjectInDynamicMemory w(new PainterParameters3dDialog(
                max_thread_count, width, height, max_screen_size, default_samples_per_pixel, max_samples_per_pixel,
                precisions, default_precision_index, colors, default_color_index, parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }
        return parameters;
}
}
