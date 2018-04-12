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

#include "ui_painter_window_2d.h"

#include <QImage>
#include <QLabel>
#include <QSlider>
#include <QTimer>
#include <deque>
#include <memory>
#include <string>
#include <vector>

class PainterWindow2d : public QWidget
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
        PainterWindow2d(const std::string& title, std::vector<int>&& m_screen_size,
                        const std::vector<int>& initial_slider_positions);
        ~PainterWindow2d() override;

protected:
        void error_message(const std::string& msg) const noexcept;

private:
        struct DimensionSlider
        {
                QLabel label;
                QSlider slider;
        };

        void showEvent(QShowEvent* event) override;
        void closeEvent(QCloseEvent* event) override;

        void init_interface(const std::vector<int>& initial_slider_positions);
        std::vector<int> slider_positions() const;
        void update_points();
        void update_statistics();

        virtual void painter_statistics(long long* pass_count, long long* pixel_count, long long* ray_count,
                                        long long* sample_count, double* previous_pass_duration) const noexcept = 0;
        virtual void slider_positions_change_event(const std::vector<int>& slider_positions) = 0;
        virtual const quint32* pixel_pointer(bool show_threads) const noexcept = 0;

        const std::vector<int> m_screen_size;
        const int m_width, m_height;
        QImage m_image;
        const int m_image_byte_count;
        QTimer m_timer;
        bool m_first_show;

        class Difference;
        std::unique_ptr<Difference> m_difference;

        std::deque<DimensionSlider> m_dimension_sliders;

        Ui::PainterWindow ui;
};
