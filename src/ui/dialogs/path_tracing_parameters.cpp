/*
Copyright (C) 2017 Topological Manifold

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

#include "path_tracing_parameters.h"

#include "message_box.h"

#include "com/print.h"

constexpr double MAX_SIZE_COEFFICIENT = 10;

PathTracingParameters::PathTracingParameters(QWidget* parent) : QDialog(parent)
{
        ui.setupUi(this);
        setWindowTitle("Path tracing parameters");
}

[[nodiscard]] bool PathTracingParameters::show(int max_thread_count, int width, int height, int* thread_count, double* size_coef)
{
        m_width = width;
        m_height = height;
        m_max_thread_count = std::max(1, max_thread_count);

        ui.spinBox_thread_count->setMinimum(1);
        ui.spinBox_thread_count->setMaximum(m_max_thread_count);
        ui.spinBox_thread_count->setValue(m_max_thread_count);

        ui.doubleSpinBox_image_size->setMinimum(1);
        ui.doubleSpinBox_image_size->setMaximum(MAX_SIZE_COEFFICIENT);
        ui.doubleSpinBox_image_size->setValue(1);
        ui.doubleSpinBox_image_size->setSingleStep(0.1);
        ui.doubleSpinBox_image_size->setDecimals(1);

        ui.label_width->setText(to_string(m_width).c_str());
        ui.label_height->setText(to_string(m_height).c_str());

        if (!this->exec())
        {
                return false;
        }

        *thread_count = m_thread_count;
        *size_coef = m_size_coef;

        return true;
}

void PathTracingParameters::on_doubleSpinBox_image_size_valueChanged(double)
{
        int width = std::round(m_width * ui.doubleSpinBox_image_size->value());
        int height = std::round(m_height * ui.doubleSpinBox_image_size->value());

        ui.label_width->setText(to_string(width).c_str());
        ui.label_height->setText(to_string(height).c_str());
}

void PathTracingParameters::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        m_thread_count = ui.spinBox_thread_count->value();
        if (!(m_thread_count >= 1 && m_thread_count <= m_max_thread_count))
        {
                std::string msg = "Error thread count. Must be in the range [1, " + to_string(m_max_thread_count) + "].";
                message_critical(this, msg.c_str());
                return;
        }

        m_size_coef = ui.doubleSpinBox_image_size->value();
        if (!(m_size_coef >= 1))
        {
                message_critical(this, "Error size coefficient. Must be equal or greater than 1.");
                return;
        }

        QDialog::done(r);
}
