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

#include <src/com/thread.h>
#include <src/com/time.h>

#include <atomic>
#include <mutex>

namespace ns::painter
{
class PaintingStatistics
{
        static_assert(std::atomic<long long>::is_always_lock_free);

        const long long m_screen_pixel_count;

        std::atomic<long long> m_pixel_count;
        std::atomic<long long> m_ray_count;
        std::atomic<long long> m_sample_count;

        long long m_pass_number;
        TimePoint m_pass_start_time;
        long long m_pass_start_pixel_count;
        double m_previous_pass_duration;

        mutable SpinLock m_lock;

public:
        PaintingStatistics(long long screen_pixel_count) : m_screen_pixel_count(screen_pixel_count)
        {
                init();
        }

        void init()
        {
                std::lock_guard lg(m_lock);

                m_pixel_count = 0;
                m_ray_count = 0;
                m_sample_count = 0;

                m_pass_number = 1;
                m_pass_start_time = time();
                m_pass_start_pixel_count = 0;
                m_previous_pass_duration = 0;
        }

        void pixel_done(int ray_count, int sample_count)
        {
                m_pixel_count.fetch_add(1, std::memory_order_relaxed);
                m_ray_count.fetch_add(ray_count, std::memory_order_relaxed);
                m_sample_count.fetch_add(sample_count, std::memory_order_relaxed);
        }

        void pass_done(bool prepare_next)
        {
                const TimePoint now = time();

                std::lock_guard lg(m_lock);

                const double previous_pass_duration = duration(m_pass_start_time, now);

                m_previous_pass_duration = previous_pass_duration;
                if (prepare_next)
                {
                        ++m_pass_number;
                        m_pass_start_time = now;
                        m_pass_start_pixel_count = m_pixel_count;
                }
        }

        Statistics statistics() const
        {
                Statistics s;

                std::lock_guard lg(m_lock);

                s.pass_number = m_pass_number;
                s.pass_progress = static_cast<double>(m_pixel_count - m_pass_start_pixel_count) / m_screen_pixel_count;
                s.previous_pass_duration = m_previous_pass_duration;

                s.pixel_count = m_pixel_count;
                s.ray_count = m_ray_count;
                s.sample_count = m_sample_count;

                return s;
        }
};
}
