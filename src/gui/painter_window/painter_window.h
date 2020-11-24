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

#include "actions.h"
#include "painter_window_2d.h"
#include "pixels.h"

#include "../com/main_thread.h"
#include "../com/support.h"
#include "../com/threads.h"

#include <src/com/error.h>
#include <src/com/global_index.h>
#include <src/com/message.h>
#include <src/painter/paintbrushes/bar_paintbrush.h>
#include <src/painter/painter.h>

#include <array>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

namespace gui::painter_window
{
template <size_t N, typename T>
class PainterWindow final : public painter_window_implementation::PainterWindow2d,
                            public painter::PainterNotifier<N - 1>
{
        static_assert(N >= 3);

        static constexpr int PANTBRUSH_WIDTH = 20;
        static constexpr size_t N_IMAGE = N - 1;

        const std::thread::id m_thread_id = std::this_thread::get_id();

        const std::shared_ptr<const painter::Scene<N, T>> m_scene;
        painter::BarPaintbrush<N_IMAGE> m_paintbrush;
        const GlobalIndex<N_IMAGE, long long> m_global_index;

        const std::vector<int> m_screen_size;
        const long long m_pixel_count;
        std::vector<long long> m_busy_indices_2d;
        Pixels m_pixels;

        std::atomic_bool m_painting_stop;
        std::thread m_painting_thread;

        std::unique_ptr<Actions> m_actions;

        //

        template <typename Type, size_t Size>
        static std::vector<Type> array_to_vector(const std::array<Type, Size>& array)
        {
                std::vector<Type> vector(Size);
                std::copy(array.cbegin(), array.cend(), vector.begin());
                return vector;
        }

        static std::vector<int> initial_slider_positions()
        {
                return std::vector<int>(N_IMAGE - 2, 0);
        }

        //

        long long pixel_index(const std::array<int_least16_t, N_IMAGE>& pixel) const
        {
                return m_global_index.compute(pixel);
        }

        long long pixel_index_for_sliders(const std::vector<int>& slider_positions) const
        {
                ASSERT(slider_positions.size() + 2 == N_IMAGE);
                std::array<int_least16_t, N_IMAGE> pixel;
                pixel[0] = 0;
                pixel[1] = 0;
                for (unsigned dimension = 2; dimension < N_IMAGE; ++dimension)
                {
                        pixel[dimension] = slider_positions[dimension - 2];
                        ASSERT(pixel[dimension] >= 0 && pixel[dimension] < m_screen_size[dimension]);
                }
                return pixel_index(pixel);
        }

        // PainterWindow2d

        Statistics statistics() const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                const painter::Statistics sp = m_paintbrush.statistics();
                Statistics s;
                s.pass_number = sp.pass_number;
                s.pass_progress = static_cast<double>(sp.pass_pixel_count) / m_pixel_count;
                s.pixel_count = sp.pixel_count;
                s.ray_count = sp.ray_count;
                s.sample_count = sp.sample_count;
                s.previous_pass_duration = sp.previous_pass_duration;
                return s;
        }

        void slider_positions_change_event(const std::vector<int>& slider_positions) override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_pixels.set_slice_offset(pixel_index_for_sliders(slider_positions));
        }

        std::span<const std::byte> pixels_bgra_2d() const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                return m_pixels.slice();
        }

        const std::vector<long long>& busy_indices_2d() const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                return m_busy_indices_2d;
        }

        void timer_event() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_actions->set_progresses();
        }

        // IPainterNotifier

        void painter_pixel_before(unsigned thread_number, const std::array<int_least16_t, N_IMAGE>& pixel) override
        {
                long long x = pixel[0];
                long long y = m_screen_size[1] - 1 - pixel[1];
                m_busy_indices_2d[thread_number] = y * m_screen_size[0] + x;
        }

        void painter_pixel_after(
                unsigned /*thread_number*/,
                const std::array<int_least16_t, N_IMAGE>& pixel,
                const Color& color,
                float coverage) override
        {
                std::array<int_least16_t, N_IMAGE> p = pixel;
                p[1] = m_screen_size[1] - 1 - pixel[1];
                m_pixels.set(pixel_index(p), color, coverage);
        }

        void painter_error_message(const std::string& msg) override
        {
                MESSAGE_ERROR(msg);
        }

public:
        PainterWindow(
                const std::string& name,
                unsigned thread_count,
                int samples_per_pixel,
                bool smooth_normal,
                const std::shared_ptr<const painter::Scene<N, T>>& scene)
                : PainterWindow2d(name, array_to_vector(scene->projector().screen_size()), initial_slider_positions()),
                  m_scene(scene),
                  m_paintbrush(m_scene->projector().screen_size(), PANTBRUSH_WIDTH, -1),
                  m_global_index(m_scene->projector().screen_size()),
                  m_screen_size(array_to_vector(m_scene->projector().screen_size())),
                  m_pixel_count(multiply_all<long long>(m_screen_size)),
                  m_busy_indices_2d(thread_count, -1),
                  m_pixels(
                          m_screen_size,
                          m_scene->background_color(),
                          pixel_index_for_sliders(initial_slider_positions())),
                  m_actions(std::make_unique<Actions>(m_screen_size, &m_pixels, menu(), status_bar()))
        {
                m_painting_stop = false;
                m_painting_thread = std::thread(
                        [=, this]()
                        {
                                paint(this, samples_per_pixel, *m_scene, &m_paintbrush, thread_count, &m_painting_stop,
                                      smooth_normal);
                        });
        }

        ~PainterWindow() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                m_actions.reset();

                m_painting_stop = true;
                if (m_painting_thread.joinable())
                {
                        m_painting_thread.join();
                }
        }

        PainterWindow(const PainterWindow&) = delete;
        PainterWindow(PainterWindow&&) = delete;
        PainterWindow& operator=(const PainterWindow&) = delete;
        PainterWindow& operator=(PainterWindow&&) = delete;
};

template <size_t N, typename T>
void create_painter_window(
        const std::string& name,
        unsigned thread_count,
        int samples_per_pixel,
        bool smooth_normal,
        std::unique_ptr<const painter::Scene<N, T>>&& scene)
{
        MainThread::run(
                [=, scene = std::shared_ptr<const painter::Scene<N, T>>(std::move(scene))]()
                {
                        create_and_show_delete_on_close_window<PainterWindow<N, T>>(
                                name, thread_count, samples_per_pixel, smooth_normal, scene);
                });
}
}
