/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "sliders_widget.h"

#include "../com/support.h"

#include <src/com/error.h>
#include <src/com/print.h>

#include <QGridLayout>
#include <QSignalBlocker>
#include <vector>

namespace ns::gui::painter_window
{
SlidersWidget::SlidersWidget(const std::vector<int>& screen_size)
{
        const int slider_count = static_cast<int>(screen_size.size()) - 2;
        if (slider_count <= 0)
        {
                error("Creating sliders for screen dimension " + to_string(screen_size.size()));
        }

        const std::vector<int> positions(slider_count, 0);

        auto* const layout = new QGridLayout(this);
        layout->setContentsMargins(5, 5, 5, 5);

        sliders_.reserve(slider_count);
        slider_positions_.reserve(slider_count);
        for (int number = 0; number < slider_count; ++number)
        {
                const int dimension = number + 2;
                const int dimension_max_value = screen_size[dimension] - 1;

                ASSERT(positions[number] >= 0 && positions[number] <= dimension_max_value);

                auto* const slider = new QSlider(this);
                slider->setOrientation(Qt::Horizontal);
                slider->setMinimum(0);
                slider->setMaximum(dimension_max_value);
                slider->setValue(positions[number]);

                auto* const label = new QLabel(this);
                set_label_minimum_width_for_text(label, to_string_digit_groups(dimension_max_value));
                label->setText(QString::fromStdString(to_string_digit_groups(positions[number])));

                const QString label_d_text = QString::fromStdString("d[" + to_string(dimension + 1) + "]");
                auto* const label_d = new QLabel(label_d_text, this);
                auto* const label_e = new QLabel("=", this);

                layout->addWidget(label_d, number, 0);
                layout->addWidget(label_e, number, 1);
                layout->addWidget(label, number, 2);
                layout->addWidget(slider, number, 3);

                sliders_.emplace(slider, Slider{.label = label, .number = static_cast<unsigned>(number)});
                slider_positions_.push_back(slider->value());
                ASSERT(slider_positions_.back() == positions[number]);

                connect(slider, &QSlider::valueChanged, this, &SlidersWidget::on_slider_changed);
        }
}

void SlidersWidget::on_slider_changed(int)
{
        const auto iter = sliders_.find(qobject_cast<QSlider*>(sender()));
        ASSERT(iter != sliders_.cend());
        ASSERT(iter->second.number < slider_positions_.size());

        const int value = iter->first->value();
        set_label_text_and_minimum_width(iter->second.label, to_string_digit_groups(value));
        slider_positions_[iter->second.number] = value;

        Q_EMIT changed(slider_positions_);
}

void SlidersWidget::set(const std::vector<int>& positions)
{
        {
                ASSERT(positions.size() == sliders_.size());
                const QSignalBlocker blocker(this);
                for (const auto& [slider, info] : sliders_)
                {
                        ASSERT(slider->minimum() <= positions[info.number]);
                        ASSERT(positions[info.number] <= slider->maximum());
                        slider->setValue(positions[info.number]);
                }
        }
        Q_EMIT changed(slider_positions_);
}
}
