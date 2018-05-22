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

#include "path_tracing_3d.h"

#include "com/error.h"
#include "com/names.h"
#include "com/print.h"
#include "ui/dialogs/messages/message_box.h"
#include "ui/support/support.h"

#include <QPointer>

// Это диалоговое окно предназначено только для 3 измерений
constexpr int DIMENSION = 3;

namespace path_tracing_parameters_for_3d_implementation
{
PathTracingParametersFor3d::PathTracingParametersFor3d(QWidget* parent) : QDialog(parent)
{
        ui.setupUi(this);
        setWindowTitle("Path Tracing");
}

bool PathTracingParametersFor3d::show(int max_thread_count, int width, int height, int max_screen_size,
                                      int default_samples_per_pixel, int max_samples_per_pixel, int* thread_count,
                                      int* paint_width, int* paint_height, int* samples_per_pixel, bool* flat_facets,
                                      bool* cornell_box)
{
        if (!(max_thread_count >= 1))
        {
                error("Error max thread count parameter: " + to_string(max_thread_count));
        }
        if (!(width >= 1 && height >= 1))
        {
                error("Error width and height parameters: width = " + to_string(width) + ", height = " + to_string(height));
        }
        if (!(max_screen_size >= 1))
        {
                error("Error max screen size parameter:  " + to_string(max_screen_size));
        }
        if (!(1 <= default_samples_per_pixel && default_samples_per_pixel <= max_samples_per_pixel))
        {
                error("Error samples per pixel parameters: max = " + to_string(max_samples_per_pixel) +
                      ", default = " + to_string(default_samples_per_pixel));
        }

        m_aspect_ratio = static_cast<double>(width) / height;
        m_max_width = m_aspect_ratio >= 1 ? max_screen_size : std::lround(max_screen_size * m_aspect_ratio);
        m_max_height = m_aspect_ratio >= 1 ? std::lround(max_screen_size / m_aspect_ratio) : max_screen_size;
        m_min_width = std::min(m_max_width, width);
        m_min_height = std::min(m_max_height, height);

        m_max_thread_count = max_thread_count;
        m_max_samples_per_pixel = max_samples_per_pixel;

        ui.label_space->setText(space_name(DIMENSION).c_str());

        ui.spinBox_thread_count->setMinimum(1);
        ui.spinBox_thread_count->setMaximum(max_thread_count);
        ui.spinBox_thread_count->setValue(max_thread_count);

        ui.spinBox_width->setMinimum(m_min_width);
        ui.spinBox_width->setMaximum(m_max_width);
        ui.spinBox_width->setValue(m_min_width);
        ui.spinBox_width->setSingleStep(std::max(1, m_min_width / 10));

        ui.spinBox_height->setMinimum(m_min_height);
        ui.spinBox_height->setMaximum(m_max_height);
        ui.spinBox_height->setValue(m_min_height);
        ui.spinBox_height->setSingleStep(std::max(1, m_min_height / 10));

        ui.spinBox_samples_per_pixel->setMinimum(1);
        ui.spinBox_samples_per_pixel->setMaximum(max_samples_per_pixel);
        ui.spinBox_samples_per_pixel->setValue(default_samples_per_pixel);

        connect(ui.spinBox_width, SIGNAL(valueChanged(int)), this, SLOT(width_value_changed(int)));
        connect(ui.spinBox_height, SIGNAL(valueChanged(int)), this, SLOT(height_value_changed(int)));

        ui.checkBox_flat_facets->setChecked(false);
        ui.checkBox_cornell_box->setChecked(false);

        if (QPointer ptr(this); !this->exec() || ptr.isNull())
        {
                return false;
        }

        *thread_count = m_thread_count;
        *paint_width = m_width;
        *paint_height = m_height;
        *samples_per_pixel = m_samples_per_pixel;
        *flat_facets = m_flat_facets;
        *cornell_box = m_cornell_box;

        return true;
}

void PathTracingParametersFor3d::done(int r)
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
                dialog::message_critical(this, msg.c_str());
                return;
        }

        m_samples_per_pixel = ui.spinBox_samples_per_pixel->value();
        if (!(m_samples_per_pixel >= 1 && m_samples_per_pixel <= m_max_samples_per_pixel))
        {
                std::string msg =
                        "Error samples per pixel. Must be in the range [1, " + to_string(m_max_samples_per_pixel) + "].";
                dialog::message_critical(this, msg.c_str());
                return;
        }

        m_width = ui.spinBox_width->value();
        if (!(m_min_width <= m_width && m_width <= m_max_width))
        {
                std::string msg = "Error width " + to_string(m_width) + ", min = " + to_string(m_min_width) +
                                  ", max = " + to_string(m_max_width);
                dialog::message_critical(this, msg.c_str());
                return;
        }

        m_height = ui.spinBox_height->value();
        if (!(m_min_height <= m_height && m_height <= m_max_height))
        {
                std::string msg = "Error height " + to_string(m_height) + ", min = " + to_string(m_min_height) +
                                  ", max = " + to_string(m_max_height);
                dialog::message_critical(this, msg.c_str());
                return;
        }

        m_flat_facets = ui.checkBox_flat_facets->isChecked();
        m_cornell_box = ui.checkBox_cornell_box->isChecked();

        QDialog::done(r);
}

void PathTracingParametersFor3d::width_value_changed(int)
{
        disconnect(ui.spinBox_height, SIGNAL(valueChanged(int)), this, SLOT(height_value_changed(int)));

        int height = std::lround(ui.spinBox_width->value() / m_aspect_ratio);
        ui.spinBox_height->setValue(std::clamp(height, m_min_height, m_max_height));

        connect(ui.spinBox_height, SIGNAL(valueChanged(int)), this, SLOT(height_value_changed(int)));
}

void PathTracingParametersFor3d::height_value_changed(int)
{
        disconnect(ui.spinBox_width, SIGNAL(valueChanged(int)), this, SLOT(width_value_changed(int)));

        int width = std::lround(ui.spinBox_height->value() * m_aspect_ratio);
        ui.spinBox_width->setValue(std::clamp(width, m_min_width, m_max_width));

        connect(ui.spinBox_width, SIGNAL(valueChanged(int)), this, SLOT(width_value_changed(int)));
}
}

namespace dialog
{
bool path_tracing_parameters_for_3d(QWidget* parent, int max_thread_count, int width, int height, int max_screen_size,
                                    int default_samples_per_pixel, int max_samples_per_pixel, int* thread_count, int* paint_width,
                                    int* paint_height, int* samples_per_pixel, bool* flat_facets, bool* cornell_box)
{
        QtObjectInDynamicMemory<path_tracing_parameters_for_3d_implementation::PathTracingParametersFor3d> w(parent);
        return w->show(max_thread_count, width, height, max_screen_size, default_samples_per_pixel, max_samples_per_pixel,
                       thread_count, paint_width, paint_height, samples_per_pixel, flat_facets, cornell_box);
}
}
