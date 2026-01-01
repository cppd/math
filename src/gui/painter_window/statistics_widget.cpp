/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "difference.h"

#include <src/com/print.h>
#include <src/gui/com/support.h>
#include <src/painter/painter.h>

#include <QWidget>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace ns::gui::painter_window
{
namespace
{
constexpr int DIFFERENCE_INTERVAL_IN_UPDATES = 10;

std::string progress_to_string(const double progress)
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
        long long pixel_count;
        long long ray_count;
        long long sample_count;

        Counters(const long long pixel_count, const long long ray_count, const long long sample_count)
                : pixel_count(pixel_count),
                  ray_count(ray_count),
                  sample_count(sample_count)
        {
        }

        Counters operator-(const Counters& c) const
        {
                return {pixel_count - c.pixel_count, ray_count - c.ray_count, sample_count - c.sample_count};
        }
};

StatisticsWidget::StatisticsWidget(const std::chrono::milliseconds& update_interval)
        : QWidget(nullptr),
          difference_(std::make_unique<Difference<Counters>>(DIFFERENCE_INTERVAL_IN_UPDATES * update_interval))
{
        ui_.setupUi(this);

        layout()->setContentsMargins(5, 5, 5, 5);

        ui_.label_rays_per_second->setText("");
        ui_.label_ray_count->setText("");
        ui_.label_pass_count->setText("");
        ui_.label_samples_per_pixel->setText("");
        ui_.label_milliseconds_per_frame->setText("");
}

StatisticsWidget::~StatisticsWidget() = default;

void StatisticsWidget::update(const painter::Statistics& statistics, const std::optional<float>& pixel_max)
{
        static constexpr std::string_view NOT_AVAILABLE = "n/a";

        const auto [difference, duration] =
                difference_->compute(Counters(statistics.pixel_count, statistics.ray_count, statistics.sample_count));

        if (duration != 0)
        {
                const long long rays_per_second = std::llround(static_cast<double>(difference.ray_count) / duration);
                com::set_label_text_and_minimum_width(
                        ui_.label_rays_per_second, to_string_digit_groups(rays_per_second));
        }
        else
        {
                com::set_label_text_and_minimum_width(ui_.label_rays_per_second, NOT_AVAILABLE);
        }

        com::set_label_text_and_minimum_width(ui_.label_ray_count, to_string_digit_groups(statistics.ray_count));

        com::set_label_text_and_minimum_width(
                ui_.label_pass_count,
                to_string_digit_groups(statistics.pass_number)
                        .append(":")
                        .append(progress_to_string(statistics.pass_progress)));

        if (difference.pixel_count != 0)
        {
                const long long samples_per_pixel =
                        std::llround(static_cast<double>(difference.sample_count) / difference.pixel_count);
                com::set_label_text_and_minimum_width(
                        ui_.label_samples_per_pixel, to_string_digit_groups(samples_per_pixel));
        }
        else
        {
                com::set_label_text_and_minimum_width(ui_.label_samples_per_pixel, NOT_AVAILABLE);
        }

        if (statistics.previous_pass_duration > 0)
        {
                com::set_label_text_and_minimum_width(
                        ui_.label_milliseconds_per_frame,
                        to_string_digit_groups(std::llround(1000 * statistics.previous_pass_duration)));
        }
        else
        {
                com::set_label_text_and_minimum_width(ui_.label_milliseconds_per_frame, NOT_AVAILABLE);
        }

        if (pixel_max)
        {
                com::set_label_text_and_minimum_width(ui_.label_max, to_string_fixed(*pixel_max, 3));
        }
        else
        {
                com::set_label_text_and_minimum_width(ui_.label_max, NOT_AVAILABLE);
        }
}
}
