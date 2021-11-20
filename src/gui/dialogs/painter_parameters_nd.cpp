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

#include "painter_parameters_nd.h"

#include "message.h"

#include "../com/support.h"

#include <src/com/error.h>
#include <src/com/names.h>
#include <src/com/print.h>

namespace ns::gui::dialog
{
PainterParametersNdDialog::PainterParametersNdDialog(
        int dimension,
        int max_thread_count,
        int default_screen_size,
        int min_screen_size,
        int max_screen_size,
        int default_samples_per_pixel,
        int max_samples_per_pixel,
        const std::array<const char*, 2>& precisions,
        int default_precision_index,
        const std::array<const char*, 2>& colors,
        int default_color_index,
        std::optional<std::tuple<PainterParameters, PainterParametersNd>>& parameters)
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
        if (!(dimension >= 4))
        {
                error("Dimension " + to_string(dimension) + " must be greater than or equal to 4");
        }

        if (!(1 <= min_screen_size))
        {
                error("Minumum screen size " + to_string(min_screen_size) + " must be positive");
        }

        if (!(min_screen_size <= max_screen_size))
        {
                error("Maximum screen size " + to_string(max_screen_size)
                      + " must be greater than or equal to minimum screen size " + to_string(min_screen_size));
        }

        if (!(min_screen_size <= default_screen_size && default_screen_size <= max_screen_size))
        {
                error("Initial screen size " + to_string(default_screen_size) + +" must be in the range ["
                      + to_string(min_screen_size) + ", " + to_string(max_screen_size) + "]");
        }

        ui_.setupUi(this);
        setWindowTitle("Painter");

        min_screen_size_ = min_screen_size;
        max_screen_size_ = max_screen_size;

        ui_.label_space->setText(QString::fromStdString(space_name(dimension)));

        ui_.spinBox_max_size->setMinimum(min_screen_size);
        ui_.spinBox_max_size->setMaximum(max_screen_size);
        ui_.spinBox_max_size->setValue(default_screen_size);

        ui_.verticalLayout_parameters->addWidget(parameters_widget_);
}

void PainterParametersNdDialog::done(int r)
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

        int max_size = ui_.spinBox_max_size->value();
        if (!(max_size >= min_screen_size_ && max_size <= max_screen_size_))
        {
                std::string msg = "Maximum screen size must be in the range [" + to_string(min_screen_size_) + ", "
                                  + to_string(max_screen_size_) + "].";
                dialog::message_critical(msg);
                return;
        }

        parameters_.emplace();

        std::get<0>(*parameters_) = parameters_widget_->parameters();
        std::get<1>(*parameters_).max_size = max_size;

        QDialog::done(r);
}

std::optional<std::tuple<PainterParameters, PainterParametersNd>> PainterParametersNdDialog::show(
        int dimension,
        int max_thread_count,
        int default_screen_size,
        int min_screen_size,
        int max_screen_size,
        int default_samples_per_pixel,
        int max_samples_per_pixel,
        const std::array<const char*, 2>& precisions,
        int default_precision_index,
        const std::array<const char*, 2>& colors,
        int default_color_index)
{
        std::optional<std::tuple<PainterParameters, PainterParametersNd>> parameters;

        QtObjectInDynamicMemory w(new PainterParametersNdDialog(
                dimension, max_thread_count, default_screen_size, min_screen_size, max_screen_size,
                default_samples_per_pixel, max_samples_per_pixel, precisions, default_precision_index, colors,
                default_color_index, parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }
        return parameters;
}
}
