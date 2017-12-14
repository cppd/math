/*
Copyright (C) 2017 Topological Manifold

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

#include "painter_window.h"

#include "com/error.h"
#include "com/print.h"
#include "com/time.h"
#include "ui/dialogs/message_box.h"

#include <QFileDialog>
#include <cstring>
#include <deque>

constexpr int UPDATE_INTERVAL_MILLISECONDS = 100;
constexpr int PANTBRUSH_WIDTH = 20;
// Этот интервал должен быть больше интервала UPDATE_INTERVAL_MILLISECONDS
constexpr int DIFFERENCE_INTERVAL_MILLISECONDS = 10 * UPDATE_INTERVAL_MILLISECONDS;

constexpr bool SHOW_THREADS = true;

namespace
{
void set_text_and_minimum_width(QLabel* label, const std::string& text)
{
        label->setText(text.c_str());
        label->setMinimumWidth(std::max(label->width(), label->fontMetrics().width(text.c_str())));
}
}

class PainterWindow::Difference
{
        struct Point
        {
                std::array<long long, 3> data;
                double time;
                Point(std::array<long long, 3> data_, double time_) : data(data_), time(time_)
                {
                }
        };

        const double m_interval_seconds;
        std::deque<Point> m_deque;

public:
        Difference(int interval_milliseconds) : m_interval_seconds(interval_milliseconds / 1000.0)
        {
        }

        std::tuple<long long, long long, long long, double> compute(const std::array<long long, 3>& data)
        {
                double time = get_time_seconds();

                // Удаление старых элементов
                while (!m_deque.empty() && m_deque.front().time < time - m_interval_seconds)
                {
                        m_deque.pop_front();
                }

                m_deque.emplace_back(data, time);

                return std::make_tuple(
                        m_deque.back().data[0] - m_deque.front().data[0], m_deque.back().data[1] - m_deque.front().data[1],
                        m_deque.back().data[2] - m_deque.front().data[2], m_deque.back().time - m_deque.front().time);
        }
};

PainterWindow::PainterWindow(const std::string& title, unsigned thread_count, std::unique_ptr<const PaintObjects>&& paint_objects)
        : m_paint_objects(std::move(paint_objects)),
          m_thread_count(thread_count),
          m_width(m_paint_objects->projector().screen_width()),
          m_height(m_paint_objects->projector().screen_height()),
          m_image(m_width, m_height, QImage::Format_RGB32),
          m_data(m_width * m_height),
          m_data_clean(m_width * m_height),
          m_image_data_size(m_width * m_height * sizeof(quint32)),
          m_first_show(true),
          m_stop(false),
          m_ray_count(0),
          m_sample_count(0),
          m_thread_working(false),
          m_window_thread_id(std::this_thread::get_id()),
          m_paintbrush(m_width, m_height, PANTBRUSH_WIDTH),
          m_difference(std::make_unique<Difference>(DIFFERENCE_INTERVAL_MILLISECONDS))
{
        ui.setupUi(this);

        ASSERT(m_image_data_size == m_data.size() * sizeof(m_data[0]));
        ASSERT(m_image_data_size == m_data_clean.size() * sizeof(m_data_clean[0]));

        connect(this, SIGNAL(error_message_signal(QString)), this, SLOT(error_message_slot(QString)),
                Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

        connect(&m_timer, SIGNAL(timeout()), this, SLOT(timer_slot()));

        ui.label_points->setText("");
        ui.label_points->resize(m_width, m_height);

        ui.label_rays_per_second->setText("");
        ui.label_ray_count->setText("");
        ui.label_pass_count->setText("");
        ui.label_samples_per_pixel->setText("");

        ui.scrollAreaWidgetContents->layout()->setContentsMargins(0, 0, 0, 0);
        ui.scrollAreaWidgetContents->layout()->setSpacing(0);
        this->layout()->setContentsMargins(5, 5, 5, 5);

        this->setWindowTitle(title.c_str());

        ui.checkBox_show_threads->setChecked(SHOW_THREADS);

        set_default_pixels();

        update_points();
}

PainterWindow::~PainterWindow()
{
        ASSERT(std::this_thread::get_id() == m_window_thread_id);

        m_stop = true;

        if (m_thread.joinable())
        {
                m_thread.join();
        }
}

void PainterWindow::set_default_pixels()
{
        for (int y = 0; y < m_height; ++y)
        {
                for (int x = 0; x < m_width; ++x)
                {
                        if ((y + x) & 1)
                        {
                                set_pixel(x, y, 100, 150, 200);
                        }
                        else
                        {
                                set_pixel(x, y, 0, 0, 0);
                        }
                }
        }
}

void PainterWindow::set_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b) noexcept
{
        quint32 c = (r << 16) + (g << 8) + b;
        int index = m_width * y + x;

        m_data[index] = c;
        m_data_clean[index] = c;
}

void PainterWindow::mark_pixel_busy(int x, int y) noexcept
{
        m_data[m_width * y + x] ^= 0x00ff'ffff;
}

void PainterWindow::update_points()
{
        const quint32* image_data = ui.checkBox_show_threads->isChecked() ? m_data.data() : m_data_clean.data();
        std::memcpy(m_image.bits(), image_data, m_image_data_size);
        ui.label_points->setPixmap(QPixmap::fromImage(m_image));
        ui.label_points->update();
}

void PainterWindow::painter_pixel_before(int x, int y) noexcept
{
        mark_pixel_busy(x, y);
}

void PainterWindow::painter_pixel_after(int x, int y, std::array<unsigned char, 3> rgb) noexcept
{
        set_pixel(x, y, rgb[0], rgb[1], rgb[2]);
}

void PainterWindow::painter_error_message(const std::string& msg) noexcept
{
        try
        {
                emit error_message_signal(msg.c_str());
        }
        catch (...)
        {
                error_fatal("Error painter message emit signal");
        }
}

void PainterWindow::error_message_slot(QString msg)
{
        message_critical(this, msg);
}

void PainterWindow::showEvent(QShowEvent* e)
{
        QWidget::showEvent(e);

        if (!m_first_show)
        {
                return;
        }
        m_first_show = false;

        QTimer::singleShot(50, this, SLOT(first_shown()));
}

void PainterWindow::first_shown()
{
        ui.scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui.scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        resize(QSize(2 + m_width, 2 + m_height) + (geometry().size() - ui.scrollArea->size()));

        ui.scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        ui.scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        m_timer.start(UPDATE_INTERVAL_MILLISECONDS);

        m_stop = false;
        m_thread_working = true;
        m_thread = std::thread([this]() noexcept {
                paint(this, *m_paint_objects, &m_paintbrush, m_thread_count, &m_stop, &m_ray_count, &m_sample_count);
                m_thread_working = false;
        });
}

void PainterWindow::timer_slot()
{
        long long pass_count = m_paintbrush.get_pass_count();

        long long ray_count = m_ray_count;
        long long sample_count = m_sample_count;
        long long pixel_count = m_paintbrush.get_pixel_count();

        auto[ray_diff, sample_diff, pixel_diff, time_diff] = m_difference->compute({{ray_count, sample_count, pixel_count}});

        long long rays_per_second = time_diff != 0 ? std::llround(ray_diff / time_diff) : 0;
        long long samples_per_pixel = pixel_diff != 0 ? std::llround(static_cast<double>(sample_diff) / pixel_diff) : 0;

        set_text_and_minimum_width(ui.label_rays_per_second, to_string_digit_groups(rays_per_second));
        set_text_and_minimum_width(ui.label_ray_count, to_string_digit_groups(ray_count));
        set_text_and_minimum_width(ui.label_pass_count, to_string_digit_groups(pass_count));
        set_text_and_minimum_width(ui.label_samples_per_pixel, to_string_digit_groups(samples_per_pixel));

        update_points();
}

void PainterWindow::on_pushButton_save_to_file_clicked()
{
        QString file_name = QFileDialog::getSaveFileName(this, "Export to file", "", "Images (*.png)", nullptr,
                                                         QFileDialog::DontUseNativeDialog);
        if (file_name.size() == 0)
        {
                return;
        }

        // Таймер и эта функция работают в одном потоке, поэтому можно пользоваться
        // переменной m_image без блокировок.
        std::memcpy(m_image.bits(), m_data_clean.data(), m_image_data_size);
        if (!m_image.save(file_name, "PNG"))
        {
                message_critical(this, "Error saving image to file");
        }
}
