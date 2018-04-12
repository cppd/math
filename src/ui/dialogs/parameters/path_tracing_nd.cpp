/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "path_tracing_nd.h"

#include "com/error.h"
#include "com/names.h"
#include "com/print.h"
#include "ui/dialogs/messages/message_box.h"

PathTracingParametersForNd::PathTracingParametersForNd(QWidget* parent) : QDialog(parent)
{
        ui.setupUi(this);
        setWindowTitle("Path Tracing");
}

bool PathTracingParametersForNd::show(int dimension, int max_thread_count, int default_screen_size, int min_screen_size,
                                      int max_screen_size, int default_samples_per_pixel, int max_samples_per_pixel,
                                      int* thread_count, int* min_size, int* max_size, int* samples_per_pixel)
{
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
                error("Error screen size parameters: min = " + to_string(min_screen_size) +
                      ", max = " + to_string(max_screen_size) + ", default = " + to_string(default_screen_size));
        }
        if (!(1 <= default_samples_per_pixel && default_samples_per_pixel <= max_samples_per_pixel))
        {
                error("Error samples per pixel parameters: max = " + to_string(max_samples_per_pixel) +
                      ", default = " + to_string(default_samples_per_pixel));
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

        if (!this->exec())
        {
                return false;
        }

        *thread_count = m_thread_count;
        *min_size = m_min_size;
        *max_size = m_max_size;
        *samples_per_pixel = m_samples_per_pixel;

        return true;
}

void PathTracingParametersForNd::on_spinBox_min_size_valueChanged(int)
{
        int min = ui.spinBox_min_size->value();
        if (min > ui.spinBox_max_size->value())
        {
                ui.spinBox_max_size->setValue(min);
        }
}

void PathTracingParametersForNd::on_spinBox_max_size_valueChanged(int)
{
        int max = ui.spinBox_max_size->value();
        if (max < ui.spinBox_min_size->value())
        {
                ui.spinBox_min_size->setValue(max);
        }
}

void PathTracingParametersForNd::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        m_thread_count = ui.spinBox_threads->value();
        if (!(m_thread_count >= 1 && m_thread_count <= m_max_thread_count))
        {
                std::string msg = "Error thread count. Must be in the range [1, " + to_string(m_max_thread_count) + "].";
                message_critical(this, msg.c_str());
                return;
        }

        m_samples_per_pixel = ui.spinBox_samples_per_pixel->value();
        if (!(m_samples_per_pixel >= 1 && m_samples_per_pixel <= m_max_samples_per_pixel))
        {
                std::string msg =
                        "Error samples per pixel. Must be in the range [1, " + to_string(m_max_samples_per_pixel) + "].";
                message_critical(this, msg.c_str());
                return;
        }

        m_min_size = ui.spinBox_min_size->value();
        if (!(m_min_size >= m_min_screen_size && m_min_size <= m_max_screen_size))
        {
                std::string msg = "Error min size. Must be in the range [" + to_string(m_min_screen_size) + ", " +
                                  to_string(m_max_screen_size) + "].";
                message_critical(this, msg.c_str());
                return;
        }

        m_max_size = ui.spinBox_max_size->value();
        if (!(m_max_size >= m_min_screen_size && m_max_size <= m_max_screen_size))
        {
                std::string msg = "Error max size. Must be in the range [" + to_string(m_min_screen_size) + ", " +
                                  to_string(m_max_screen_size) + "].";
                message_critical(this, msg.c_str());
                return;
        }

        if (!(m_min_size <= m_max_size))
        {
                std::string msg = "Error min and max sizes. The min size must be less than the max size or equal to the max size";
                message_critical(this, msg.c_str());
                return;
        }

        QDialog::done(r);
}
