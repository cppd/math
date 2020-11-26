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
#include "image_widget.h"
#include "pixels.h"
#include "sliders_widget.h"
#include "statistics_widget.h"

#include "../com/main_thread.h"
#include "../com/support.h"

#include "ui_painter_window.h"

#include <src/painter/painter.h>

#include <QTimer>
#include <memory>
#include <string>
#include <thread>

namespace gui::painter_window
{
class PainterWindow final : public QMainWindow
{
        Q_OBJECT

private:
        const std::thread::id m_thread_id = std::this_thread::get_id();
        bool m_first_show = true;

        Ui::PainterWindow ui;

        QAction* m_show_threads_action = nullptr;

        std::unique_ptr<Pixels> m_pixels;

        std::unique_ptr<ImageWidget> m_image_widget;
        std::unique_ptr<StatisticsWidget> m_statistics_widget;
        std::unique_ptr<SlidersWidget> m_sliders_widget;

        std::unique_ptr<Actions> m_actions;

        QTimer m_timer;

        void on_timer_timeout();
        void on_first_shown();

        void showEvent(QShowEvent* event) override;
        void closeEvent(QCloseEvent* event) override;

        void create_menu();
        void create_interface();
        void create_sliders();

        void adjust_window_size();

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
