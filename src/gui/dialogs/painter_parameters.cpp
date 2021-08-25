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

#include "painter_parameters.h"

#include "message.h"

#include <src/com/error.h>
#include <src/com/print.h>

namespace ns::gui::dialog
{
PainterParametersWidget::PainterParametersWidget(
        QWidget* parent,
        int max_thread_count,
        int default_samples_per_pixel,
        int max_samples_per_pixel,
        const std::array<const char*, 2>& precisions,
        int default_precision_index,
        const std::array<const char*, 2>& colors,
        int default_color_index)
        : QWidget(parent)
{
        if (!(max_thread_count >= 1))
        {
                error("Maximum thread count " + to_string(max_thread_count) + " must be greater than or equal to 1");
        }

        if (!(1 <= max_samples_per_pixel))
        {
                error("Maximum samples per pixel " + to_string(max_samples_per_pixel)
                      + " must be greater than or equal to 1");
        }

        if (!(1 <= default_samples_per_pixel && default_samples_per_pixel <= max_samples_per_pixel))
        {
                error("Initial samples per pixel " + to_string(default_samples_per_pixel) + " must be in the range [1, "
                      + to_string(max_samples_per_pixel) + "]");
        }

        if (std::any_of(
                    precisions.cbegin(), precisions.cend(),
                    [](const char* s)
                    {
                            return s == nullptr || std::string_view(s).empty();
                    }))
        {
                error("Empty precisions");
        }

        if (!(default_precision_index == 0 || default_precision_index == 1))
        {
                error("Default precision index error " + to_string(default_precision_index));
        }

        if (std::any_of(
                    colors.cbegin(), colors.cend(),
                    [](const char* s)
                    {
                            return s == nullptr || std::string_view(s).empty();
                    }))
        {
                error("Empty colors");
        }

        if (!(default_color_index == 0 || default_color_index == 1))
        {
                error("Default color index error " + to_string(default_color_index));
        }

        ui_.setupUi(this);

        this->layout()->setContentsMargins(0, 0, 0, 0);

        max_thread_count_ = max_thread_count;
        max_samples_per_pixel_ = max_samples_per_pixel;

        ui_.spinBox_threads->setMinimum(1);
        ui_.spinBox_threads->setMaximum(max_thread_count);
        ui_.spinBox_threads->setValue(max_thread_count);

        ui_.spinBox_samples_per_pixel->setMinimum(1);
        ui_.spinBox_samples_per_pixel->setMaximum(max_samples_per_pixel);
        ui_.spinBox_samples_per_pixel->setValue(default_samples_per_pixel);

        ui_.checkBox_flat_facets->setChecked(false);
        ui_.checkBox_cornell_box->setChecked(false);

        ui_.radioButton_precision_0->setText(precisions[0]);
        ui_.radioButton_precision_1->setText(precisions[1]);
        if (default_precision_index == 0)
        {
                ui_.radioButton_precision_0->setChecked(true);
        }
        else
        {
                ui_.radioButton_precision_1->setChecked(true);
        }

        ui_.radioButton_color_0->setText(colors[0]);
        ui_.radioButton_color_1->setText(colors[1]);
        if (default_color_index == 0)
        {
                ui_.radioButton_color_0->setChecked(true);
        }
        else
        {
                ui_.radioButton_color_1->setChecked(true);
        }
}

bool PainterParametersWidget::check()
{
        int thread_count = ui_.spinBox_threads->value();
        if (!(thread_count >= 1 && thread_count <= max_thread_count_))
        {
                std::string msg = "Thread count must be in the range [1, " + to_string(max_thread_count_) + "].";
                dialog::message_critical(msg);
                return false;
        }

        int samples_per_pixel = ui_.spinBox_samples_per_pixel->value();
        if (!(samples_per_pixel >= 1 && samples_per_pixel <= max_samples_per_pixel_))
        {
                std::string msg =
                        "Samples per pixel must be in the range [1, " + to_string(max_samples_per_pixel_) + "].";
                dialog::message_critical(msg);
                return false;
        }

        if (!(ui_.radioButton_precision_0->isChecked() || ui_.radioButton_precision_1->isChecked()))
        {
                dialog::message_critical("Precision is not selected");
                return false;
        }

        if (!(ui_.radioButton_color_0->isChecked() || ui_.radioButton_color_1->isChecked()))
        {
                dialog::message_critical("Color is not selected");
                return false;
        }

        return true;
}

[[nodiscard]] PainterParameters PainterParametersWidget::parameters()
{
        PainterParameters p;
        p.thread_count = ui_.spinBox_threads->value();
        p.samples_per_pixel = ui_.spinBox_samples_per_pixel->value();
        p.flat_facets = ui_.checkBox_flat_facets->isChecked();
        p.cornell_box = ui_.checkBox_cornell_box->isChecked();
        p.precision_index = ui_.radioButton_precision_0->isChecked() ? 0 : 1;
        p.color_index = ui_.radioButton_color_0->isChecked() ? 0 : 1;
        return p;
}
}
