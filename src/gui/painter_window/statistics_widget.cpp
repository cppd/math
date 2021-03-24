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

#include "statistics_widget.h"

#include "../com/support.h"

#include <src/com/print.h>

#include <algorithm>
#include <cmath>

namespace ns::gui::painter_window
{
namespace
{
std::string progress_to_string(double progress)
{
        const int percent = std::clamp(static_cast<int>(progress * 100), 0, 100);
        std::string str;
        if (percent < 10)
        {
                str += ' ';
                str += ' ';
        }
        else if (percent < 100)
        {
                str += ' ';
        }
        str += std::to_string(percent);
        str += '%';
        return str;
}
}

struct StatisticsWidget::Counters final
{
        const long long pixel_count;
        const long long ray_count;
        const long long sample_count;

        Counters(long long pixel_count, long long ray_count, long long sample_count)
                : pixel_count(pixel_count), ray_count(ray_count), sample_count(sample_count)
        {
        }

        Counters operator-(const Counters& c) const
        {
                return Counters(pixel_count - c.pixel_count, ray_count - c.ray_count, sample_count - c.sample_count);
        }
};

StatisticsWidget::StatisticsWidget(std::chrono::milliseconds update_interval)
        : QWidget(nullptr),
          // Здесь интервал должен быть больше update_interval_milliseconds
          m_difference(std::make_unique<Difference<Counters>>(10 * update_interval))
{
        ui.setupUi(this);

        layout()->setContentsMargins(5, 5, 5, 5);

        ui.label_rays_per_second->setText("");
        ui.label_ray_count->setText("");
        ui.label_pass_count->setText("");
        ui.label_samples_per_pixel->setText("");
        ui.label_milliseconds_per_frame->setText("");
}

StatisticsWidget::~StatisticsWidget() = default;

void StatisticsWidget::update(const painter::Statistics& statistics)
{
        auto [difference, duration] =
                m_difference->compute(Counters(statistics.pixel_count, statistics.ray_count, statistics.sample_count));

        long long rays_per_second =
                (duration != 0) ? std::llround(static_cast<double>(difference.ray_count) / duration) : 0;

        long long samples_per_pixel =
                (difference.pixel_count != 0)
                        ? std::llround(static_cast<double>(difference.sample_count) / difference.pixel_count)
                        : 0;

        long long milliseconds_per_frame = std::llround(1000 * statistics.previous_pass_duration);

        set_label_text_and_minimum_width(ui.label_rays_per_second, to_string_digit_groups(rays_per_second));
        set_label_text_and_minimum_width(ui.label_ray_count, to_string_digit_groups(statistics.ray_count));
        set_label_text_and_minimum_width(
                ui.label_pass_count, to_string_digit_groups(statistics.pass_number)
                                             .append(":")
                                             .append(progress_to_string(statistics.pass_progress)));
        set_label_text_and_minimum_width(ui.label_samples_per_pixel, to_string_digit_groups(samples_per_pixel));
        set_label_text_and_minimum_width(
                ui.label_milliseconds_per_frame, to_string_digit_groups(milliseconds_per_frame));
}
}
