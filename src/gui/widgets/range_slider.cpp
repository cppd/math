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

#include "range_slider.h"

#include <src/com/error.h>

#include <cmath>
#include <limits>
#include <numeric>

namespace gui
{
namespace
{
constexpr int MIN = 0;
constexpr int MAX = 500;

static_assert(MIN >= 0 && MIN < MAX);
}

RangeSlider::RangeSlider(QWidget* parent) : QWidget(parent)
{
        ui.setupUi(this);

        ui.slider_min->setInvertedAppearance(true);
        ui.slider_min->setMinimum(MIN);
        ui.slider_min->setMaximum(MAX);
        ui.slider_max->setMinimum(ui.slider_min->minimum());
        ui.slider_max->setMaximum(ui.slider_min->maximum());

        ui.slider_min->setTracking(true);
        ui.slider_max->setTracking(true);

        set_range(0, 1);

        m_last_min = std::numeric_limits<double>::lowest();
        m_last_max = std::numeric_limits<double>::lowest();

        connect(ui.slider_min, SIGNAL(valueChanged(int)), this, SLOT(min_value_changed(int)));
        connect(ui.slider_max, SIGNAL(valueChanged(int)), this, SLOT(max_value_changed(int)));
}

void RangeSlider::set_range(double min, double max)
{
        ASSERT(std::isfinite(min) && std::isfinite(max));

        min = std::clamp(min, 0.0, 1.0);
        max = std::clamp(max, 0.0, 1.0);
        if (min > max)
        {
                max = min = std::midpoint(min, max);
        }

        QSignalBlocker blocker_min(ui.slider_min);
        QSignalBlocker blocker_max(ui.slider_max);

        set_min_value(std::lround(std::lerp(MIN, MAX, min)));
        set_max_value(std::lround(std::lerp(MIN, MAX, max)));

        ASSERT(min_value() <= max_value());

        range_changed();
}

int RangeSlider::min_value() const
{
        return ui.slider_min->maximum() - ui.slider_min->value();
}

int RangeSlider::max_value() const
{
        return ui.slider_max->value();
}

void RangeSlider::set_min_value(int value)
{
        ui.slider_min->setValue(ui.slider_min->maximum() - value);
}

void RangeSlider::set_max_value(int value)
{
        ui.slider_max->setValue(value);
}

void RangeSlider::min_value_changed(int)
{
        if (min_value() > max_value())
        {
                set_min_value(max_value());
        }
        range_changed();
}

void RangeSlider::max_value_changed(int)
{
        if (max_value() < min_value())
        {
                set_max_value(min_value());
        }
        range_changed();
}

void RangeSlider::range_changed()
{
        constexpr double d = MAX - MIN;
        const double min = (min_value() - MIN) / d;
        const double max = (max_value() - MIN) / d;

        ASSERT(min <= max);

        if (m_last_min != min || m_last_max != max)
        {
                m_last_min = min;
                m_last_max = max;
                emit range_changed(min, max);
        }
}
}
