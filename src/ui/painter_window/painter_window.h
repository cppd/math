/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "ui_painter_window.h"

#include "com/thread.h"
#include "path_tracing/paintbrushes/paintbrush.h"
#include "path_tracing/painter.h"

#include <QTimer>
#include <memory>
#include <string>
#include <thread>

class PainterWindow final : public QWidget, public IPainterNotifier
{
        Q_OBJECT

public:
        PainterWindow(const std::string& title, unsigned thread_count, int samples_per_pixel,
                      std::unique_ptr<const PaintObjects>&& paint_objects);
        ~PainterWindow() override;

signals:
        void error_message_signal(QString) const;

private slots:
        void timer_slot();
        void first_shown();
        void error_message_slot(QString);

        void on_pushButton_save_to_file_clicked();

private:
        void showEvent(QShowEvent* event) override;

        void painter_pixel_before(int x, int y) noexcept override;
        void painter_pixel_after(int x, int y, const SrgbInteger& c) noexcept override;
        void painter_error_message(const std::string& msg) noexcept override;

        void set_default_pixels();
        void set_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) noexcept;
        void mark_pixel_busy(int x, int y) noexcept;
        void update_points();

        int m_samples_per_pixel;
        std::unique_ptr<const PaintObjects> m_paint_objects;
        unsigned m_thread_count;
        int m_width, m_height;
        QImage m_image;
        std::vector<quint32> m_data, m_data_clean;
        const unsigned m_image_data_size;
        QTimer m_timer;
        bool m_first_show;
        std::atomic_bool m_stop;
        std::thread m_thread;
        std::atomic_bool m_thread_working;
        const std::thread::id m_window_thread_id;
        BarPaintbrush m_paintbrush;

        class Difference;
        std::unique_ptr<Difference> m_difference;

        Ui::PainterWindow ui;
};
