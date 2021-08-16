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

#pragma once

#include "../painter.h"

#include <src/com/spin_lock.h>
#include <src/com/time.h>

#include <atomic>
#include <mutex>

namespace ns::painter
{
class PaintingStatistics
{
        static_assert(std::atomic<long long>::is_always_lock_free);

        const long long screen_pixel_count_;

        std::atomic<long long> pixel_counter_;
        std::atomic<long long> ray_counter_;
        std::atomic<long long> sample_counter_;

        long long pass_number_;
        TimePoint pass_start_time_;
        long long pass_start_pixel_count_;
        double previous_pass_duration_;

        mutable SpinLock lock_;

public:
        explicit PaintingStatistics(long long screen_pixel_count) : screen_pixel_count_(screen_pixel_count)
        {
                init();
        }

        void init()
        {
                std::lock_guard lg(lock_);

                pixel_counter_ = 0;
                ray_counter_ = 0;
                sample_counter_ = 0;

                pass_number_ = 1;
                pass_start_time_ = time();
                pass_start_pixel_count_ = 0;
                previous_pass_duration_ = 0;
        }

        void pixel_done(int ray_count, int sample_count)
        {
                pixel_counter_.fetch_add(1, std::memory_order_relaxed);
                ray_counter_.fetch_add(ray_count, std::memory_order_relaxed);
                sample_counter_.fetch_add(sample_count, std::memory_order_relaxed);
        }

        void pass_done()
        {
                const TimePoint now = time();
                std::lock_guard lg(lock_);
                previous_pass_duration_ = duration(pass_start_time_, now);
        }

        void next_pass()
        {
                std::lock_guard lg(lock_);
                ++pass_number_;
                pass_start_time_ = time();
                pass_start_pixel_count_ = pixel_counter_;
        }

        Statistics statistics() const
        {
                Statistics s;

                std::lock_guard lg(lock_);

                s.pass_number = pass_number_;
                s.pass_progress = static_cast<double>(pixel_counter_ - pass_start_pixel_count_) / screen_pixel_count_;
                s.previous_pass_duration = previous_pass_duration_;

                s.pixel_count = pixel_counter_;
                s.ray_count = ray_counter_;
                s.sample_count = sample_counter_;

                return s;
        }
};
}
