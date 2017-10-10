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

constexpr int UPDATE_INTERVAL = 100;
constexpr int PANTBRUSH_WIDTH = 20;

constexpr bool SHOW_THREADS = true;

// Количество лучей в секунду
class PainterWindow::RPS
{
        struct Point
        {
                long long count;
                double time;
                Point(long long count_, double time_) : count(count_), time(time_)
                {
                }
        };
        std::deque<Point> m_deque;
        double m_interval;

public:
        RPS(double interval) : m_interval(interval)
        {
        }

        double freq(long long count)
        {
                double time = get_time_seconds();

                m_deque.emplace_back(count, time);

                // Удаление старых элементов
                while (m_deque.size() >= 2 && m_deque.front().time < time - m_interval)
                {
                        m_deque.pop_front();
                }

                return (m_deque.size() >= 2) ? (count - m_deque.front().count) / (time - m_deque.front().time) : 0;
        }
};

PainterWindow::PainterWindow(const std::string& title, unsigned thread_count, std::unique_ptr<const PaintObjects>&& paint_objects)
        : m_paint_objects(std::move(paint_objects)),
          m_thread_count(thread_count),
          m_width(m_paint_objects->get_projector().screen_width()),
          m_height(m_paint_objects->get_projector().screen_height()),
          m_image(m_width, m_height, QImage::Format_RGB32),
          m_data(m_width * m_height),
          m_data_clean(m_width * m_height),
          m_image_data_size(m_width * m_height * sizeof(quint32)),
          m_first_show(true),
          m_stop(false),
          m_ray_count(0),
          m_thread_working(false),
          m_window_thread_id(std::this_thread::get_id()),
          m_paintbrush(m_width, m_height, PANTBRUSH_WIDTH),
          m_rps(std::make_unique<RPS>(1))
{
        ui.setupUi(this);

        setAttribute(Qt::WA_DeleteOnClose);

        ASSERT(m_image_data_size == m_data.size() * sizeof(m_data[0]));
        ASSERT(m_image_data_size == m_data_clean.size() * sizeof(m_data_clean[0]));

        connect(this, SIGNAL(error_message_signal(QString)), this, SLOT(error_message_slot(QString)),
                Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

        connect(&m_timer, SIGNAL(timeout()), this, SLOT(timer_slot()));

        for (int y = 0; y < m_height; ++y)
        {
                for (int x = 0; x < m_width; ++x)
                {
                        if ((y + x) & 1)
                        {
                                set_pixel(x, y, 100, 150, 200);
                        }
                }
        }

        ui.label_points->setText("");
        ui.label_points->resize(m_width, m_height);

        ui.label_rps->setText("");
        ui.label_ray_count->setText("");
        ui.label_pass_count->setText("");

        ui.scrollAreaWidgetContents->layout()->setContentsMargins(0, 0, 0, 0);
        ui.scrollAreaWidgetContents->layout()->setSpacing(0);
        this->layout()->setContentsMargins(5, 5, 5, 5);

        this->setWindowTitle(title.c_str());

        ui.checkBox_show_threads->setChecked(SHOW_THREADS);

        update_points();

        show();
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

void PainterWindow::update_points() noexcept
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

void PainterWindow::painter_pixel_after(int x, int y, unsigned char r, unsigned char g, unsigned char b) noexcept
{
        set_pixel(x, y, r, g, b);
}

void PainterWindow::painter_error_message(const std::string& msg) noexcept
{
        try
        {
                emit error_message_signal(msg.c_str());
        }
        catch (...)
        {
                error_fatal("Error painter error message emit signal");
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

        m_timer.start(UPDATE_INTERVAL);

        m_stop = false;
        m_ray_count = 0;
        m_thread_working = true;
        m_thread = std::thread([this]() {
                paint(this, m_paint_objects.get(), &m_paintbrush, m_thread_count, &m_stop, &m_ray_count);
                m_thread_working = false;
        });
}

void PainterWindow::timer_slot()
{
        long long ray_count = m_ray_count;
        long long rps = std::round(m_rps->freq(ray_count));
        long long pass_count = m_paintbrush.get_pass_count();

        ui.label_rps->setText(to_string_digit_groups(rps).c_str());
        ui.label_ray_count->setText(to_string_digit_groups(ray_count).c_str());
        ui.label_pass_count->setText(to_string_digit_groups(pass_count).c_str());

        ui.label_rps->setMinimumWidth(ui.label_rps->width());
        ui.label_ray_count->setMinimumWidth(ui.label_ray_count->width());
        ui.label_pass_count->setMinimumWidth(ui.label_pass_count->width());

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

void create_painter_window(const std::string& title, unsigned thread_count, std::unique_ptr<const PaintObjects>&& paint_objects)
{
        // В окне вызывается setAttribute(Qt::WA_DeleteOnClose),
        // поэтому можно просто new.
        new PainterWindow(title, thread_count, std::move(paint_objects));
}
