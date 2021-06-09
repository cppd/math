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
// Это диалоговое окно предназначено только для 3 измерений
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
        std::optional<PainterParameters3d>& parameters)
        : QDialog(parent_for_dialog()),
          m_parameters_widget(new PainterParametersWidget(
                  this,
                  max_thread_count,
                  default_samples_per_pixel,
                  max_samples_per_pixel,
                  precisions,
                  default_precision_index,
                  colors,
                  default_color_index)),
          m_parameters(parameters)
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

        ui.setupUi(this);
        setWindowTitle("Painter");

        connect(ui.spinBox_width, QOverload<int>::of(&QSpinBox::valueChanged), this,
                &PainterParameters3dDialog::on_width_value_changed);
        connect(ui.spinBox_height, QOverload<int>::of(&QSpinBox::valueChanged), this,
                &PainterParameters3dDialog::on_height_value_changed);

        m_aspect_ratio = static_cast<double>(width) / height;
        m_max_width = m_aspect_ratio >= 1 ? max_screen_size : std::lround(max_screen_size * m_aspect_ratio);
        m_max_height = m_aspect_ratio >= 1 ? std::lround(max_screen_size / m_aspect_ratio) : max_screen_size;
        m_min_width = std::min(m_max_width, width);
        m_min_height = std::min(m_max_height, height);

        ui.label_space->setText(QString::fromStdString(space_name(DIMENSION)));

        ui.spinBox_width->setMinimum(m_min_width);
        ui.spinBox_width->setMaximum(m_max_width);
        ui.spinBox_width->setValue(m_min_width);
        ui.spinBox_width->setSingleStep(std::max(1, m_min_width / 10));

        ui.spinBox_height->setMinimum(m_min_height);
        ui.spinBox_height->setMaximum(m_max_height);
        ui.spinBox_height->setValue(m_min_height);
        ui.spinBox_height->setSingleStep(std::max(1, m_min_height / 10));

        ui.verticalLayout_parameters->addWidget(m_parameters_widget);
}

void PainterParameters3dDialog::on_width_value_changed(int)
{
        int height = std::lround(ui.spinBox_width->value() / m_aspect_ratio);

        QSignalBlocker blocker(ui.spinBox_height);
        ui.spinBox_height->setValue(std::clamp(height, m_min_height, m_max_height));
}

void PainterParameters3dDialog::on_height_value_changed(int)
{
        int width = std::lround(ui.spinBox_height->value() * m_aspect_ratio);

        QSignalBlocker blocker(ui.spinBox_width);
        ui.spinBox_width->setValue(std::clamp(width, m_min_width, m_max_width));
}

void PainterParameters3dDialog::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        if (!m_parameters_widget->check())
        {
                return;
        }

        int width = ui.spinBox_width->value();
        if (!(m_min_width <= width && width <= m_max_width))
        {
                std::string msg =
                        "Width must be in the range [" + to_string(m_min_width) + ", " + to_string(m_max_width) + "]";
                dialog::message_critical(msg);
                return;
        }

        int height = ui.spinBox_height->value();
        if (!(m_min_height <= height && height <= m_max_height))
        {
                std::string msg = "Height must be in the range [" + to_string(m_min_height) + ", "
                                  + to_string(m_max_height) + "]";
                dialog::message_critical(msg);
                return;
        }

        m_parameters.emplace();

        m_parameters->width = width;
        m_parameters->height = height;
        m_parameters->thread_count = m_parameters_widget->thread_count();
        m_parameters->samples_per_pixel = m_parameters_widget->samples_per_pixel();
        m_parameters->flat_facets = m_parameters_widget->flat_facets();
        m_parameters->cornell_box = m_parameters_widget->cornell_box();
        m_parameters->precision_index = m_parameters_widget->precision_index();
        m_parameters->color_index = m_parameters_widget->color_index();

        QDialog::done(r);
}

std::optional<PainterParameters3d> PainterParameters3dDialog::show(
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
        std::optional<PainterParameters3d> parameters;

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
