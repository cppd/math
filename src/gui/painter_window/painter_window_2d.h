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

#include "ui_painter_window_2d.h"

#include <QImage>
#include <QLabel>
#include <QSlider>
#include <QTimer>
#include <deque>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace gui::painter_window_implementation
{
struct Statistics final
{
        long long pass_count;
        long long pixel_count;
        long long ray_count;
        long long sample_count;
        double previous_pass_duration;
};

class PainterWindow2d : public QWidget
{
        Q_OBJECT

private:
        const std::thread::id m_window_thread_id = std::this_thread::get_id();
        bool m_first_show = true;

        Ui::PainterWindow ui;

        const std::vector<int> m_screen_size;
        const int m_width;
        const int m_height;
        const long long m_image_pixel_count;
        const long long m_image_byte_count;
        QImage m_image;
        QTimer m_timer;

        class Difference;
        std::unique_ptr<Difference> m_difference;

        struct DimensionSlider
        {
                QLabel label;
                QSlider slider;
        };
        std::deque<DimensionSlider> m_dimension_sliders;

        void on_save_to_file_clicked();
        void on_add_volume_clicked();

        void on_timer_timeout();
        void on_first_shown();
        void on_slider_changed(int);

        void showEvent(QShowEvent* event) override;
        void closeEvent(QCloseEvent* event) override;

        void init_interface(const std::vector<int>& initial_slider_positions);
        std::vector<int> slider_positions() const;
        void update_points();
        void update_statistics();

        virtual Statistics statistics() const = 0;
        virtual void slider_positions_change_event(const std::vector<int>& slider_positions) = 0;
        virtual const std::vector<std::uint_least32_t>& pixels_bgr() const = 0;
        virtual long long pixels_offset() const = 0;
        virtual const std::vector<long long>& pixels_busy() const = 0;
        virtual void save_to_file() const = 0;
        virtual void add_volume() const = 0;

public:
        PainterWindow2d(
                const std::string& name,
                std::vector<int>&& m_screen_size,
                const std::vector<int>& initial_slider_positions);
        ~PainterWindow2d() override;
};
}
