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

#include "statistics_widget.h"

#include "../com/support.h"

#include <src/com/print.h>

#include <cmath>

namespace gui::painter_window
{
namespace
{
std::string progress_to_string(const char* prefix, double progress)
{
        int percent = std::floor(progress * 100.0);
        percent = (percent < 100) ? percent : 99;
        std::string progress_str = prefix;
        progress_str += percent < 10 ? "0" : "";
        progress_str += std::to_string(percent);
        return progress_str;
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

StatisticsWidget::StatisticsWidget(const Pixels* pixels, std::chrono::milliseconds update_interval)
        : QWidget(nullptr),
          m_pixels(pixels),
          m_pixel_count(multiply_all<long long>(m_pixels->screen_size())),
          // Здесь интервал должен быть больше update_interval_milliseconds
          m_difference(std::make_unique<Difference<Counters>>(10 * update_interval))
{
        ui.setupUi(this);

        layout()->setContentsMargins(0, 0, 0, 0);

        ui.label_rays_per_second->setText("");
        ui.label_ray_count->setText("");
        ui.label_pass_count->setText("");
        ui.label_samples_per_pixel->setText("");
        ui.label_milliseconds_per_frame->setText("");
}

StatisticsWidget::~StatisticsWidget() = default;

void StatisticsWidget::update()
{
        const painter::Statistics s = m_pixels->statistics();

        auto [difference, duration] = m_difference->compute(Counters(s.pixel_count, s.ray_count, s.sample_count));

        long long rays_per_second =
                (duration != 0) ? std::llround(static_cast<double>(difference.ray_count) / duration) : 0;

        long long samples_per_pixel =
                (difference.pixel_count != 0)
                        ? std::llround(static_cast<double>(difference.sample_count) / difference.pixel_count)
                        : 0;

        long long milliseconds_per_frame = std::llround(1000 * s.previous_pass_duration);

        double pass_progress = static_cast<double>(s.pass_pixel_count) / m_pixel_count;

        set_label_text_and_minimum_width(ui.label_rays_per_second, to_string_digit_groups(rays_per_second));
        set_label_text_and_minimum_width(ui.label_ray_count, to_string_digit_groups(s.ray_count));
        set_label_text_and_minimum_width(
                ui.label_pass_count,
                to_string_digit_groups(s.pass_number).append(progress_to_string(":", pass_progress)));
        set_label_text_and_minimum_width(ui.label_samples_per_pixel, to_string_digit_groups(samples_per_pixel));
        set_label_text_and_minimum_width(
                ui.label_milliseconds_per_frame, to_string_digit_groups(milliseconds_per_frame));
}
}
