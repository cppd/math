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

RangeSlider::RangeSlider(
        QSlider* slider_min,
        QSlider* slider_max,
        const std::function<void(double, double)>& range_changed)
        : m_slider_min(slider_min),
          m_slider_max(slider_max),
          m_last_min(std::numeric_limits<decltype(m_last_min)>::lowest()),
          m_last_max(std::numeric_limits<decltype(m_last_max)>::lowest()),
          m_range_changed(range_changed)
{
        ASSERT(m_slider_min);
        ASSERT(m_slider_max);
        ASSERT(m_range_changed);

        m_slider_min->setInvertedAppearance(true);
        m_slider_min->setMinimum(MIN);
        m_slider_min->setMaximum(MAX);
        m_slider_max->setMinimum(m_slider_min->minimum());
        m_slider_max->setMaximum(m_slider_min->maximum());

        m_slider_min->setTracking(true);
        m_slider_max->setTracking(true);

        set_range(0, 1);

        m_connections.emplace_back(
                QObject::connect(m_slider_min, &QSlider::valueChanged, [&](int) { min_value_changed(); }));

        m_connections.emplace_back(
                QObject::connect(m_slider_max, &QSlider::valueChanged, [&](int) { max_value_changed(); }));
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

        QSignalBlocker blocker_min(m_slider_min);
        QSignalBlocker blocker_max(m_slider_max);

        set_min_value(std::lround(std::lerp(MIN, MAX, min)));
        set_max_value(std::lround(std::lerp(MIN, MAX, max)));

        ASSERT(min_value() <= max_value());

        range_changed();
}

int RangeSlider::min_value() const
{
        return m_slider_min->maximum() - m_slider_min->value();
}

int RangeSlider::max_value() const
{
        return m_slider_max->value();
}

void RangeSlider::set_min_value(int value)
{
        m_slider_min->setValue(m_slider_min->maximum() - value);
}

void RangeSlider::set_max_value(int value)
{
        m_slider_max->setValue(value);
}

void RangeSlider::min_value_changed()
{
        if (min_value() > max_value())
        {
                set_min_value(max_value());
        }
        range_changed();
}

void RangeSlider::max_value_changed()
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
                m_range_changed(min, max);
        }
}
}
