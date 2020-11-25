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
#include "difference.h"
#include "pixels.h"

#include "../com/main_thread.h"
#include "../com/support.h"

#include "ui_painter_window.h"

#include <src/painter/painter.h>

#include <QImage>
#include <QTimer>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace gui::painter_window
{
class PainterWindow final : public QMainWindow
{
        Q_OBJECT

private:
        const std::thread::id m_thread_id = std::this_thread::get_id();
        bool m_first_show = true;

        Ui::PainterWindow ui;

        const long long m_image_2d_pixel_count;
        const long long m_image_2d_byte_count;
        QImage m_image_2d;

        struct Counters;
        std::unique_ptr<Difference<Counters>> m_difference;

        struct Slider
        {
                QLabel* label;
                unsigned number;
        };
        std::unordered_map<const QSlider*, Slider> m_sliders;
        std::vector<int> m_slider_positions;

        QAction* m_show_threads_action = nullptr;

        std::unique_ptr<Pixels> m_pixels;
        const long long m_pixel_count;

        std::unique_ptr<Actions> m_actions;

        QTimer m_timer;

        void on_timer_timeout();
        void on_first_shown();
        void on_slider_changed(int);

        void showEvent(QShowEvent* event) override;
        void closeEvent(QCloseEvent* event) override;

        void make_menu();
        void init_interface(const std::string& name);
        void make_sliders(const std::vector<int>& screen_size);

        void adjust_window_size();

        void update_points();
        void update_statistics();

public:
        PainterWindow(const std::string& name, std::unique_ptr<Pixels>&& pixels);

        ~PainterWindow() override;
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
                        create_and_show_delete_on_close_window<PainterWindow>(
                                name, std::make_unique<PainterPixels<N, T>>(
                                              scene, thread_count, samples_per_pixel, smooth_normal));
                });
}
}
