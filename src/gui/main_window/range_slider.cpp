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

#include "range_slider.h"

#include <src/com/error.h>
#include <src/com/type/limit.h>

#include <QObject>
#include <QSignalBlocker>
#include <QSlider>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace ns::gui::main_window
{
namespace
{
constexpr int MIN = 0;
constexpr int MAX = 500;

static_assert(MIN >= 0 && MIN < MAX);
}

RangeSlider::RangeSlider(QSlider* const slider_min, QSlider* const slider_max)
        : slider_min_(slider_min),
          slider_max_(slider_max),
          last_min_(Limits<decltype(last_min_)>::lowest()),
          last_max_(Limits<decltype(last_max_)>::lowest())
{
        ASSERT(slider_min_);
        ASSERT(slider_max_);

        slider_min_->setInvertedAppearance(true);
        slider_min_->setMinimum(MIN);
        slider_min_->setMaximum(MAX);
        slider_max_->setMinimum(slider_min_->minimum());
        slider_max_->setMaximum(slider_min_->maximum());

        slider_min_->setTracking(true);
        slider_max_->setTracking(true);

        set_range(0, 1);

        connections_.emplace_back(QObject::connect(
                slider_min_, &QSlider::valueChanged,
                [&](int)
                {
                        on_min_value_changed();
                }));

        connections_.emplace_back(QObject::connect(
                slider_max_, &QSlider::valueChanged,
                [&](int)
                {
                        on_max_value_changed();
                }));
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

        {
                const QSignalBlocker blocker_min(slider_min_);
                const QSignalBlocker blocker_max(slider_max_);
                set_min_value(std::lround(std::lerp(MIN, MAX, min)));
                set_max_value(std::lround(std::lerp(MIN, MAX, max)));
        }

        ASSERT(min_value() <= max_value());

        range_changed();
}

int RangeSlider::min_value() const
{
        return slider_min_->maximum() - slider_min_->value();
}

int RangeSlider::max_value() const
{
        return slider_max_->value();
}

void RangeSlider::set_min_value(const int value)
{
        slider_min_->setValue(slider_min_->maximum() - value);
}

void RangeSlider::set_max_value(const int value)
{
        slider_max_->setValue(value);
}

void RangeSlider::on_min_value_changed()
{
        if (min_value() > max_value())
        {
                set_min_value(max_value());
        }
        range_changed();
}

void RangeSlider::on_max_value_changed()
{
        if (max_value() < min_value())
        {
                set_max_value(min_value());
        }
        range_changed();
}

void RangeSlider::range_changed()
{
        constexpr double LENGTH = MAX - MIN;
        const double min = (min_value() - MIN) / LENGTH;
        const double max = (max_value() - MIN) / LENGTH;

        ASSERT(min <= max);

        if (last_min_ != min || last_max_ != max)
        {
                last_min_ = min;
                last_max_ = max;
                Q_EMIT changed(min, max);
        }
}
}
