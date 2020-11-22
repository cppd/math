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

#include "difference.h"

#include "ui_painter_window_2d.h"

#include <QImage>
#include <QLabel>
#include <QSlider>
#include <QTimer>
#include <memory>
#include <span>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace gui::painter_window::painter_window_implementation
{
class PainterWindow2d : public QMainWindow
{
        Q_OBJECT

protected:
        struct Statistics final
        {
                long long pass_number;
                double pass_progress;
                long long pixel_count;
                long long ray_count;
                long long sample_count;
                double previous_pass_duration;
        };

private:
        const std::thread::id m_thread_id = std::this_thread::get_id();
        bool m_first_show = true;

        Ui::PainterWindow ui;

        const std::vector<int> m_screen_size;
        const int m_width;
        const int m_height;
        const long long m_image_pixel_count;
        const long long m_image_byte_count;
        QImage m_image;
        QTimer m_timer;

        struct Counters;
        std::unique_ptr<Difference<Counters>> m_difference;

        struct Slider
        {
                QLabel* label;
                unsigned number;
        };
        std::unordered_map<const QSlider*, Slider> m_sliders;
        std::vector<int> m_slider_positions;

        QAction* m_show_threads = nullptr;

        void on_timer_timeout();
        void on_first_shown();
        void on_slider_changed(int);

        void on_save_with_background();
        void on_save_without_background();
        void on_save_all_with_background();
        void on_save_all_without_background();
        void on_add_volume_with_background();
        void on_add_volume_without_background();

        void showEvent(QShowEvent* event) override;
        void closeEvent(QCloseEvent* event) override;

        void make_menu();
        void init_interface(const std::string& name);
        void make_sliders(const std::vector<int>& initial_slider_positions);

        void update_points();
        void update_statistics();
        void adjust_window_size();

        virtual Statistics statistics() const = 0;
        virtual void slider_positions_change_event(const std::vector<int>& slider_positions) = 0;
        virtual std::span<const std::byte> pixels_bgra_2d() const = 0;
        virtual const std::vector<long long>& busy_indices_2d() const = 0;
        virtual void save_to_file(bool without_background) const = 0;
        virtual void save_all_to_files(bool without_background) const = 0;
        virtual void add_volume(bool without_background) const = 0;

public:
        PainterWindow2d(
                const std::string& name,
                const std::vector<int>& m_screen_size,
                const std::vector<int>& initial_slider_positions);

        ~PainterWindow2d() override;
};
}
