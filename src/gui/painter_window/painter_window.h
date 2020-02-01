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

#pragma once

#include "painter_window_2d.h"

#include <src/com/global_index.h>
#include <src/painter/painter.h>
#include <src/painter/visible_paintbrush.h>

#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

template <size_t N, typename T>
class PainterWindow final : public PainterWindow2d, public PainterNotifier<N - 1>
{
        static_assert(N >= 3);
        static constexpr size_t N_IMAGE = N - 1;

        const std::unique_ptr<const PaintObjects<N, T>> m_paint_objects;
        const GlobalIndex<N_IMAGE, long long> m_global_index;
        const int m_height;
        const std::thread::id m_window_thread_id;

        long long m_slice_offset;
        std::vector<std::uint_least32_t> m_data;

        VisibleBarPaintbrush<N_IMAGE> m_paintbrush;
        std::atomic_bool m_stop;
        std::atomic_bool m_thread_working;

        std::thread m_thread;

        std::vector<long long> m_busy_pixels;

        static std::vector<int> initial_slider_positions();

        long long pixel_index(const std::array<int_least16_t, N_IMAGE>& pixel) const;
        long long offset_for_slider_positions(const std::vector<int>& slider_positions) const;

        // PainterWindow2d
        void painter_statistics(
                long long* pass_count,
                long long* pixel_count,
                long long* ray_count,
                long long* sample_count,
                double* previous_pass_duration) const override;
        void slider_positions_change_event(const std::vector<int>& slider_positions) override;
        const std::vector<std::uint_least32_t>& pixels_bgr() const override;
        long long pixels_offset() const override;
        const std::vector<long long>& pixels_busy() const override;

        // IPainterNotifier
        void painter_pixel_before(unsigned thread_number, const std::array<int_least16_t, N_IMAGE>& pixel) override;
        void painter_pixel_after(
                unsigned thread_number,
                const std::array<int_least16_t, N_IMAGE>& pixel,
                const Color& color) override;
        void painter_error_message(const std::string& msg) override;

public:
        PainterWindow(
                const std::string& title,
                unsigned thread_count,
                int samples_per_pixel,
                bool smooth_normal,
                std::unique_ptr<const PaintObjects<N, T>>&& paint_objects);

        ~PainterWindow() override;

        PainterWindow(const PainterWindow&) = delete;
        PainterWindow(PainterWindow&&) = delete;
        PainterWindow& operator=(const PainterWindow&) = delete;
        PainterWindow& operator=(PainterWindow&&) = delete;
};
