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

#include "painter_3d.h"

#include "message.h"

#include "../com/support.h"

#include <src/com/error.h>
#include <src/com/names.h>
#include <src/com/print.h>

namespace gui::dialog
{
namespace
{
// Это диалоговое окно предназначено только для 3 измерений
constexpr int DIMENSION = 3;
}

Painter3dParametersDialog::Painter3dParametersDialog(
        int max_thread_count,
        int width,
        int height,
        int max_screen_size,
        int default_samples_per_pixel,
        int max_samples_per_pixel,
        std::optional<Painter3dParameters>& parameters)
        : QDialog(parent_for_dialog()), m_parameters(parameters)
{
        ui.setupUi(this);
        setWindowTitle("Painter");

        connect(ui.spinBox_width, QOverload<int>::of(&QSpinBox::valueChanged), this,
                &Painter3dParametersDialog::on_width_value_changed);
        connect(ui.spinBox_height, QOverload<int>::of(&QSpinBox::valueChanged), this,
                &Painter3dParametersDialog::on_height_value_changed);

        if (!(max_thread_count >= 1))
        {
                error("Error max thread count parameter: " + to_string(max_thread_count));
        }
        if (!(width >= 1 && height >= 1))
        {
                error("Error width and height parameters: width = " + to_string(width)
                      + ", height = " + to_string(height));
        }
        if (!(max_screen_size >= 1))
        {
                error("Error max screen size parameter:  " + to_string(max_screen_size));
        }
        if (!(1 <= default_samples_per_pixel && default_samples_per_pixel <= max_samples_per_pixel))
        {
                error("Error samples per pixel parameters: max = " + to_string(max_samples_per_pixel)
                      + ", default = " + to_string(default_samples_per_pixel));
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

        ui.checkBox_flat_facets->setChecked(false);
        ui.checkBox_cornell_box->setChecked(false);
}

void Painter3dParametersDialog::on_width_value_changed(int)
{
        int height = std::lround(ui.spinBox_width->value() / m_aspect_ratio);

        QSignalBlocker blocker(ui.spinBox_height);
        ui.spinBox_height->setValue(std::clamp(height, m_min_height, m_max_height));
}

void Painter3dParametersDialog::on_height_value_changed(int)
{
        int width = std::lround(ui.spinBox_height->value() * m_aspect_ratio);

        QSignalBlocker blocker(ui.spinBox_width);
        ui.spinBox_width->setValue(std::clamp(width, m_min_width, m_max_width));
}

void Painter3dParametersDialog::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        int thread_count = ui.spinBox_thread_count->value();
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

        int width = ui.spinBox_width->value();
        if (!(m_min_width <= width && width <= m_max_width))
        {
                std::string msg = "Error width " + to_string(width) + ", min = " + to_string(m_min_width)
                                  + ", max = " + to_string(m_max_width);
                dialog::message_critical(msg);
                return;
        }

        int height = ui.spinBox_height->value();
        if (!(m_min_height <= height && height <= m_max_height))
        {
                std::string msg = "Error height " + to_string(height) + ", min = " + to_string(m_min_height)
                                  + ", max = " + to_string(m_max_height);
                dialog::message_critical(msg);
                return;
        }

        m_parameters.emplace();
        m_parameters->thread_count = thread_count;
        m_parameters->samples_per_pixel = samples_per_pixel;
        m_parameters->width = width;
        m_parameters->height = height;
        m_parameters->flat_facets = ui.checkBox_flat_facets->isChecked();
        m_parameters->cornell_box = ui.checkBox_cornell_box->isChecked();

        QDialog::done(r);
}

std::optional<Painter3dParameters> Painter3dParametersDialog::show(
        int max_thread_count,
        int width,
        int height,
        int max_screen_size,
        int default_samples_per_pixel,
        int max_samples_per_pixel)
{
        std::optional<Painter3dParameters> parameters;

        QtObjectInDynamicMemory w(new Painter3dParametersDialog(
                max_thread_count, width, height, max_screen_size, default_samples_per_pixel, max_samples_per_pixel,
                parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }
        return parameters;
}
}
