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
#include "ui/dialogs/message_box.h"

#include <QTimer>
#include <cstring>

constexpr int UPDATE_INTERVAL = 100;
constexpr int PANTBRUSH_WIDTH = 20;

PainterWindow::PainterWindow(unsigned thread_count, std::unique_ptr<const PaintObjects>&& paint_objects)
        : m_paint_objects(std::move(paint_objects)),
          m_thread_count(thread_count),
          m_width(m_paint_objects->get_projector().screen_width()),
          m_height(m_paint_objects->get_projector().screen_height()),
          m_image(m_width, m_height, QImage::Format_RGB32),
          m_data(m_width * m_height),
          m_first_show(true),
          m_stop(false),
          m_ray_count(0),
          m_thread_working(false),
          m_window_thread_id(std::this_thread::get_id()),
          m_paintbrush(m_width, m_height, PANTBRUSH_WIDTH)
{
        ui.setupUi(this);

        setAttribute(Qt::WA_DeleteOnClose);

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

        ui.label_ray_count->setText("");

        ui.scrollAreaWidgetContents->layout()->setContentsMargins(0, 0, 0, 0);
        ui.scrollAreaWidgetContents->layout()->setSpacing(0);
        this->layout()->setContentsMargins(5, 5, 5, 5);
        ui.formLayout->setContentsMargins(5, 5, 5, 5);

        this->setWindowTitle("Path Tracing");

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
        m_data[m_width * y + x] = (r << 16) + (g << 8) + b;
}

void PainterWindow::xor_pixel(int x, int y) noexcept
{
        quint32* c = &m_data[m_width * y + x];
        *c ^= 0xffff'ffff;
        *c &= 0x00ff'ffff;
}

void PainterWindow::update_points() noexcept
{
        std::memcpy(m_image.bits(), m_data.data(), m_data.size() * sizeof(m_data[0]));
        ui.label_points->setPixmap(QPixmap::fromImage(m_image));
        ui.label_points->update();
}

void PainterWindow::painter_pixel_before(int x, int y) noexcept
{
        xor_pixel(x, y);
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
        ui.label_ray_count->setText(to_string(m_ray_count).c_str());

        update_points();
}

void create_painter_window(unsigned thread_count, std::unique_ptr<const PaintObjects>&& paint_objects)
{
        // В окне вызывается setAttribute(Qt::WA_DeleteOnClose),
        // поэтому можно просто new.
        new PainterWindow(thread_count, std::move(paint_objects));
}
