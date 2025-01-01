/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <QRadioButton>
#include <QWidget>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>

namespace ns::gui::dialogs
{
namespace
{
void check_texts(const char* const name, const std::array<const char*, 2>& texts, const int index)
{
        if (std::ranges::any_of(
                    texts,
                    [](const char* const s)
                    {
                            return s == nullptr || std::string_view(s).empty();
                    }))
        {
                error(std::string("Empty ") + name);
        }

        if (!(index == 0 || index == 1))
        {
                error("Index " + to_string(index) + " is out of range for " + name);
        }
}

void check_parameters(
        const int max_thread_count,
        const int samples_per_pixel,
        const int max_samples_per_pixel,
        const std::array<const char*, 2>& precisions,
        const int precision_index,
        const std::array<const char*, 2>& colors,
        const int color_index,
        const std::array<const char*, 2>& integrators,
        const int integrator_index)
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

        if (!(1 <= samples_per_pixel && samples_per_pixel <= max_samples_per_pixel))
        {
                error("Initial samples per pixel " + to_string(samples_per_pixel) + " must be in the range [1, "
                      + to_string(max_samples_per_pixel) + "]");
        }

        check_texts("precisions", precisions, precision_index);

        check_texts("colors", colors, color_index);

        check_texts("integrators", integrators, integrator_index);
}

void set_buttons(const std::array<QRadioButton*, 2>& buttons, const std::array<const char*, 2>& texts, const int index)
{
        for (std::size_t i = 0; i < 2; ++i)
        {
                buttons[i]->setText(texts[i]);
        }

        if (index < 2)
        {
                buttons[index]->setChecked(true);
                return;
        }

        error("Index " + to_string(index) + " is out of range");
}

[[nodiscard]] bool check_button_selection(const char* const name, const std::array<QRadioButton*, 2>& buttons)
{
        const auto count = std::count_if(
                buttons.cbegin(), buttons.cend(),
                [](const QRadioButton* const button)
                {
                        return button->isChecked();
                });

        if (count == 1)
        {
                return true;
        }

        dialogs::message_critical(name + std::string(" is not selected"));
        return false;
}
}

PainterParametersWidget::PainterParametersWidget(
        QWidget* const parent,
        const int max_thread_count,
        const int samples_per_pixel,
        const int max_samples_per_pixel,
        const std::array<const char*, 2>& precisions,
        const int precision_index,
        const std::array<const char*, 2>& colors,
        const int color_index,
        const std::array<const char*, 2>& integrators,
        const int integrator_index)
        : QWidget(parent),
          max_thread_count_(max_thread_count),
          max_samples_per_pixel_(max_samples_per_pixel)
{
        check_parameters(
                max_thread_count, samples_per_pixel, max_samples_per_pixel, precisions, precision_index, colors,
                color_index, integrators, integrator_index);

        ui_.setupUi(this);

        this->layout()->setContentsMargins(0, 0, 0, 0);

        ui_.spin_box_threads->setMinimum(1);
        ui_.spin_box_threads->setMaximum(max_thread_count);
        ui_.spin_box_threads->setValue(max_thread_count);

        ui_.spin_box_samples_per_pixel->setMinimum(1);
        ui_.spin_box_samples_per_pixel->setMaximum(max_samples_per_pixel);
        ui_.spin_box_samples_per_pixel->setValue(samples_per_pixel);

        ui_.check_box_flat_shading->setChecked(false);
        ui_.check_box_cornell_box->setChecked(false);

        set_buttons({ui_.radio_button_precision_0, ui_.radio_button_precision_1}, precisions, precision_index);

        set_buttons({ui_.radio_button_color_0, ui_.radio_button_color_1}, colors, color_index);

        set_buttons({ui_.radio_button_integrator_0, ui_.radio_button_integrator_1}, integrators, integrator_index);
}

bool PainterParametersWidget::check()
{
        const int thread_count = ui_.spin_box_threads->value();
        if (!(thread_count >= 1 && thread_count <= max_thread_count_))
        {
                const std::string msg = "Thread count must be in the range [1, " + to_string(max_thread_count_) + "].";
                dialogs::message_critical(msg);
                return false;
        }

        const int samples_per_pixel = ui_.spin_box_samples_per_pixel->value();
        if (!(samples_per_pixel >= 1 && samples_per_pixel <= max_samples_per_pixel_))
        {
                const std::string msg =
                        "Samples per pixel must be in the range [1, " + to_string(max_samples_per_pixel_) + "].";
                dialogs::message_critical(msg);
                return false;
        }

        if (!check_button_selection("Precision", {ui_.radio_button_precision_0, ui_.radio_button_precision_1}))
        {
                return false;
        }

        if (!check_button_selection("Color", {ui_.radio_button_color_0, ui_.radio_button_color_1}))
        {
                return false;
        }

        if (!check_button_selection("Integrator", {ui_.radio_button_integrator_0, ui_.radio_button_integrator_1}))
        {
                return false;
        }

        return true;
}

[[nodiscard]] PainterParameters PainterParametersWidget::parameters()
{
        return {.thread_count = ui_.spin_box_threads->value(),
                .samples_per_pixel = ui_.spin_box_samples_per_pixel->value(),
                .flat_shading = ui_.check_box_flat_shading->isChecked(),
                .cornell_box = ui_.check_box_cornell_box->isChecked(),
                .precision_index = ui_.radio_button_precision_0->isChecked() ? 0 : 1,
                .color_index = ui_.radio_button_color_0->isChecked() ? 0 : 1,
                .integrator_index = ui_.radio_button_integrator_0->isChecked() ? 0 : 1};
}
}
