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

#include "painter_nd.h"

#include "message.h"

#include "../com/support.h"

#include <src/com/error.h>
#include <src/com/names.h>
#include <src/com/print.h>

namespace gui::dialog
{
PainterNdParametersDialog::PainterNdParametersDialog(
        int dimension,
        int max_thread_count,
        int default_screen_size,
        int min_screen_size,
        int max_screen_size,
        int default_samples_per_pixel,
        int max_samples_per_pixel,
        std::optional<PainterNdParameters>& parameters)
        : QDialog(parent_for_dialog()), m_parameters(parameters)
{
        ui.setupUi(this);
        setWindowTitle("Painter");

        connect(ui.spinBox_min_size, QOverload<int>::of(&QSpinBox::valueChanged), this,
                &PainterNdParametersDialog::on_min_size_changed);
        connect(ui.spinBox_max_size, QOverload<int>::of(&QSpinBox::valueChanged), this,
                &PainterNdParametersDialog::on_max_size_changed);

        if (!(dimension >= 4))
        {
                error("Error dimension parameter: " + to_string(dimension));
        }
        if (!(max_thread_count >= 1))
        {
                error("Error max thread count parameter: " + to_string(max_thread_count));
        }
        if (!(1 <= min_screen_size && min_screen_size <= default_screen_size && default_screen_size <= max_screen_size))
        {
                error("Error screen size parameters: min = " + to_string(min_screen_size)
                      + ", max = " + to_string(max_screen_size) + ", default = " + to_string(default_screen_size));
        }
        if (!(1 <= default_samples_per_pixel && default_samples_per_pixel <= max_samples_per_pixel))
        {
                error("Error samples per pixel parameters: max = " + to_string(max_samples_per_pixel)
                      + ", default = " + to_string(default_samples_per_pixel));
        }

        m_max_thread_count = max_thread_count;
        m_min_screen_size = min_screen_size;
        m_max_screen_size = max_screen_size;
        m_max_samples_per_pixel = max_samples_per_pixel;

        ui.label_space->setText(space_name(dimension).c_str());

        ui.spinBox_threads->setMinimum(1);
        ui.spinBox_threads->setMaximum(max_thread_count);
        ui.spinBox_threads->setValue(max_thread_count);

        ui.spinBox_min_size->setMinimum(min_screen_size);
        ui.spinBox_min_size->setMaximum(max_screen_size);
        ui.spinBox_min_size->setValue(min_screen_size);

        ui.spinBox_max_size->setMinimum(min_screen_size);
        ui.spinBox_max_size->setMaximum(max_screen_size);
        ui.spinBox_max_size->setValue(default_screen_size);

        ui.spinBox_samples_per_pixel->setMinimum(1);
        ui.spinBox_samples_per_pixel->setMaximum(max_samples_per_pixel);
        ui.spinBox_samples_per_pixel->setValue(default_samples_per_pixel);

        ui.checkBox_flat_facets->setChecked(false);
        ui.checkBox_cornell_box->setChecked(false);
}

void PainterNdParametersDialog::on_min_size_changed(int)
{
        int min = ui.spinBox_min_size->value();
        if (min > ui.spinBox_max_size->value())
        {
                QSignalBlocker blocker(ui.spinBox_max_size);
                ui.spinBox_max_size->setValue(min);
        }
}

void PainterNdParametersDialog::on_max_size_changed(int)
{
        int max = ui.spinBox_max_size->value();
        if (max < ui.spinBox_min_size->value())
        {
                QSignalBlocker blocker(ui.spinBox_min_size);
                ui.spinBox_min_size->setValue(max);
        }
}

void PainterNdParametersDialog::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        int thread_count = ui.spinBox_threads->value();
        if (!(thread_count >= 1 && thread_count <= m_max_thread_count))
        {
                std::string msg =
                        "Error thread count. Must be in the range [1, " + to_string(m_max_thread_count) + "].";
                dialog::message_critical(msg);
                return;
        }

        int samples_per_pixel = ui.spinBox_samples_per_pixel->value();
        if (!(samples_per_pixel >= 1 && samples_per_pixel <= m_max_samples_per_pixel))
        {
                std::string msg = "Error samples per pixel. Must be in the range [1, "
                                  + to_string(m_max_samples_per_pixel) + "].";
                dialog::message_critical(msg);
                return;
        }

        int min_size = ui.spinBox_min_size->value();
        if (!(min_size >= m_min_screen_size && min_size <= m_max_screen_size))
        {
                std::string msg = "Error min size. Must be in the range [" + to_string(m_min_screen_size) + ", "
                                  + to_string(m_max_screen_size) + "].";
                dialog::message_critical(msg);
                return;
        }

        int max_size = ui.spinBox_max_size->value();
        if (!(max_size >= m_min_screen_size && max_size <= m_max_screen_size))
        {
                std::string msg = "Error max size. Must be in the range [" + to_string(m_min_screen_size) + ", "
                                  + to_string(m_max_screen_size) + "].";
                dialog::message_critical(msg);
                return;
        }

        if (!(min_size <= max_size))
        {
                std::string msg =
                        "Error min and max sizes. The min size must be less than the max size or equal to the max size";
                dialog::message_critical(msg);
                return;
        }

        m_parameters.emplace();
        m_parameters->thread_count = thread_count;
        m_parameters->samples_per_pixel = samples_per_pixel;
        m_parameters->min_size = min_size;
        m_parameters->max_size = max_size;
        m_parameters->flat_facets = ui.checkBox_flat_facets->isChecked();
        m_parameters->cornell_box = ui.checkBox_cornell_box->isChecked();

        QDialog::done(r);
}

std::optional<PainterNdParameters> PainterNdParametersDialog::show(
        int dimension,
        int max_thread_count,
        int default_screen_size,
        int min_screen_size,
        int max_screen_size,
        int default_samples_per_pixel,
        int max_samples_per_pixel)
{
        std::optional<PainterNdParameters> parameters;

        QtObjectInDynamicMemory w(new PainterNdParametersDialog(
                dimension, max_thread_count, default_screen_size, min_screen_size, max_screen_size,
                default_samples_per_pixel, max_samples_per_pixel, parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }
        return parameters;
}
}
