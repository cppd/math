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
        int default_precision_index)
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

        ui.setupUi(this);

        this->layout()->setContentsMargins(0, 0, 0, 0);

        m_max_thread_count = max_thread_count;
        m_max_samples_per_pixel = max_samples_per_pixel;

        ui.spinBox_threads->setMinimum(1);
        ui.spinBox_threads->setMaximum(max_thread_count);
        ui.spinBox_threads->setValue(max_thread_count);

        ui.spinBox_samples_per_pixel->setMinimum(1);
        ui.spinBox_samples_per_pixel->setMaximum(max_samples_per_pixel);
        ui.spinBox_samples_per_pixel->setValue(default_samples_per_pixel);

        ui.checkBox_flat_facets->setChecked(false);
        ui.checkBox_cornell_box->setChecked(false);

        ui.radioButton_precision_0->setText(precisions[0]);
        ui.radioButton_precision_1->setText(precisions[1]);
        if (default_precision_index == 0)
        {
                ui.radioButton_precision_0->setChecked(true);
        }
        else
        {
                ui.radioButton_precision_1->setChecked(true);
        }
}

bool PainterParametersWidget::check()
{
        int thread_count = ui.spinBox_threads->value();
        if (!(thread_count >= 1 && thread_count <= m_max_thread_count))
        {
                std::string msg = "Thread count must be in the range [1, " + to_string(m_max_thread_count) + "].";
                dialog::message_critical(msg);
                return false;
        }

        int samples_per_pixel = ui.spinBox_samples_per_pixel->value();
        if (!(samples_per_pixel >= 1 && samples_per_pixel <= m_max_samples_per_pixel))
        {
                std::string msg =
                        "Samples per pixel must be in the range [1, " + to_string(m_max_samples_per_pixel) + "].";
                dialog::message_critical(msg);
                return false;
        }

        if (!(ui.radioButton_precision_0->isChecked() || ui.radioButton_precision_1->isChecked()))
        {
                dialog::message_critical("Precision is not selected");
                return false;
        }

        return true;
}

int PainterParametersWidget::thread_count()
{
        return ui.spinBox_threads->value();
}

int PainterParametersWidget::samples_per_pixel()
{
        return ui.spinBox_samples_per_pixel->value();
}

bool PainterParametersWidget::flat_facets()
{
        return ui.checkBox_flat_facets->isChecked();
}

bool PainterParametersWidget::cornell_box()
{
        return ui.checkBox_cornell_box->isChecked();
}

int PainterParametersWidget::precision_index()
{
        if (ui.radioButton_precision_0->isChecked())
        {
                return 0;
        }
        return 1;
}
}
