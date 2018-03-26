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

#include "com/global_index.h"
#include "path_tracing/painter.h"
#include "path_tracing/visible_paintbrush.h"

#include <QImage>
#include <QLabel>
#include <QSlider>
#include <QTimer>
#include <deque>
#include <memory>
#include <string>
#include <thread>

class PainterWindowUI : public QWidget
{
        Q_OBJECT

signals:
        void error_message_signal(QString) const;

private slots:
        void timer_slot();
        void first_shown();
        void error_message_slot(QString);
        void slider_changed_slot(int);

        void on_pushButton_save_to_file_clicked();

public:
        PainterWindowUI(const std::string& title, std::vector<int>&& m_screen_size);
        ~PainterWindowUI() override;

protected:
        void init_window();
        void set_pixel(long long index, unsigned char r, unsigned char g, unsigned char b) noexcept;
        void mark_pixel_busy(long long index) noexcept;
        void error_message(const std::string& msg) const noexcept;

private:
        void showEvent(QShowEvent* event) override;

        void update_points();

        void set_default_pixels();
        void set_interface();
        void set_data_vectors();
        void set_slice_offset();

        virtual void painter_statistics(long long* pass_count, long long* pixel_count, long long* ray_count,
                                        long long* sample_count, double* previous_pass_duration) const noexcept = 0;
        virtual long long slice_offset(const std::vector<int>& slider_positions) const = 0;

        const std::vector<int> m_screen_size;
        const int m_width, m_height;
        QImage m_image;
        std::vector<quint32> m_data, m_data_clean;
        const int m_image_byte_count;
        QTimer m_timer;
        bool m_first_show;

        class Difference;
        std::unique_ptr<Difference> m_difference;

        struct DimensionSlider
        {
                QLabel label;
                QSlider slider;
        };
        std::deque<DimensionSlider> m_dimension_sliders;

        long long m_slice_offset;

        Ui::PainterWindow ui;
};

template <size_t N, typename T>
class PainterWindow final : public PainterWindowUI, public IPainterNotifier<N - 1>
{
        static_assert(N >= 3);
        static constexpr size_t N_IMAGE = N - 1;

        const std::unique_ptr<const PaintObjects<N, T>> m_paint_objects;
        const GlobalIndex<N_IMAGE, long long> m_global_index;
        const int m_height;
        const int m_samples_per_pixel;
        const unsigned m_thread_count;
        const std::thread::id m_window_thread_id;

        VisibleBarPaintbrush<N_IMAGE> m_paintbrush;
        std::atomic_bool m_stop;
        std::atomic_bool m_thread_working;

        std::thread m_thread;

        long long pixel_index(const std::array<int_least16_t, N_IMAGE>& pixel) const noexcept;

        // PainterWindowUI
        void painter_statistics(long long* pass_count, long long* pixel_count, long long* ray_count, long long* sample_count,
                                double* previous_pass_duration) const noexcept override;
        long long slice_offset(const std::vector<int>& slider_positions) const override;

        // IPainterNotifier
        void painter_pixel_before(const std::array<int_least16_t, N_IMAGE>& pixel) noexcept override;
        void painter_pixel_after(const std::array<int_least16_t, N_IMAGE>& pixel, const SrgbInteger& c) noexcept override;
        void painter_error_message(const std::string& msg) noexcept override;

public:
        PainterWindow(const std::string& title, unsigned thread_count, int samples_per_pixel,
                      std::unique_ptr<const PaintObjects<N, T>>&& paint_objects);

        ~PainterWindow() override;
};
