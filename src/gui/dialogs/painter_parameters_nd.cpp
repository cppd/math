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

#include "painter_parameters_nd.h"

#include "message.h"
#include "painter_parameters.h"

#include <src/com/error.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/gui/com/support.h>

#include <QDialog>
#include <QString>

#include <array>
#include <optional>
#include <string>
#include <tuple>

namespace ns::gui::dialogs
{
namespace
{
void check_parameters(const int dimension, const int screen_size, const int min_screen_size, const int max_screen_size)
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

        if (!(min_screen_size <= screen_size && screen_size <= max_screen_size))
        {
                error("Initial screen size " + to_string(screen_size) + +" must be in the range ["
                      + to_string(min_screen_size) + ", " + to_string(max_screen_size) + "]");
        }
}
}

PainterParametersNdDialog::PainterParametersNdDialog(
        const int dimension,
        const int max_thread_count,
        const int screen_size,
        const int min_screen_size,
        const int max_screen_size,
        const int samples_per_pixel,
        const int max_samples_per_pixel,
        const std::array<const char*, 2>& precisions,
        const int precision_index,
        const std::array<const char*, 2>& colors,
        const int color_index,
        const std::array<const char*, 2>& integrators,
        const int integrator_index,
        std::optional<std::tuple<PainterParameters, PainterParametersNd>>* const parameters)
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
          min_screen_size_(min_screen_size),
          max_screen_size_(max_screen_size),
          parameters_(parameters)
{
        ui_.setupUi(this);
        setWindowTitle("Painter");

        ui_.label_space->setText(QString::fromStdString(space_name(dimension)));

        ui_.spin_box_max_size->setMinimum(min_screen_size);
        ui_.spin_box_max_size->setMaximum(max_screen_size);
        ui_.spin_box_max_size->setValue(screen_size);

        ui_.vertical_layout_parameters->addWidget(parameters_widget_);

        com::set_dialog_size(this);
}

void PainterParametersNdDialog::done(const int r)
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

        const int max_size = ui_.spin_box_max_size->value();
        if (!(max_size >= min_screen_size_ && max_size <= max_screen_size_))
        {
                const std::string msg =
                        "Maximum screen size must be in the range [" + to_string(min_screen_size_) + ", "
                        + to_string(max_screen_size_) + "].";
                dialogs::message_critical(msg);
                return;
        }

        auto& parameters = parameters_->emplace();
        std::get<0>(parameters) = parameters_widget_->parameters();
        std::get<1>(parameters).max_size = max_size;

        QDialog::done(r);
}

std::optional<std::tuple<PainterParameters, PainterParametersNd>> PainterParametersNdDialog::show(
        const int dimension,
        const int max_thread_count,
        const int screen_size,
        const int min_screen_size,
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
        check_parameters(dimension, screen_size, min_screen_size, max_screen_size);

        std::optional<std::tuple<PainterParameters, PainterParametersNd>> parameters;

        const com::QtObjectInDynamicMemory w(new PainterParametersNdDialog(
                dimension, max_thread_count, screen_size, min_screen_size, max_screen_size, samples_per_pixel,
                max_samples_per_pixel, precisions, precision_index, colors, color_index, integrators, integrator_index,
                &parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }

        ASSERT(parameters);
        return parameters;
}
}
