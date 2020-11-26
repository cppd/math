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

#include "painter_window.h"

#include "../dialogs/message.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/settings/name.h>

#include <QCloseEvent>
#include <QPointer>
#include <cstring>

namespace gui::painter_window
{
namespace
{
constexpr int UPDATE_INTERVAL_MILLISECONDS = 100;
constexpr bool SHOW_THREADS = true;
}

PainterWindow::PainterWindow(const std::string& name, std::unique_ptr<Pixels>&& pixels)
        : m_image_2d_pixel_count(1ull * pixels->screen_size()[0] * pixels->screen_size()[1]),
          m_image_2d_byte_count(sizeof(quint32) * m_image_2d_pixel_count),
          m_image_2d(pixels->screen_size()[0], pixels->screen_size()[1], QImage::Format_RGB32),
          m_pixels(std::move(pixels))
{
        ASSERT(m_image_2d.sizeInBytes() == m_image_2d_byte_count);

        ui.setupUi(this);

        make_menu();
        init_interface(name);
        create_sliders(m_pixels->screen_size());

        m_actions = std::make_unique<Actions>(m_pixels.get(), ui.menu_actions, ui.status_bar);
}

PainterWindow::~PainterWindow()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        m_timer.stop();

        m_actions.reset();
        m_pixels.reset();
}

void PainterWindow::closeEvent(QCloseEvent* event)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        QPointer ptr(this);
        bool yes;
        if (!dialog::message_question_default_no("Do you want to close the painter window?", &yes) || ptr.isNull()
            || !yes)
        {
                if (!ptr.isNull())
                {
                        event->ignore();
                }
                return;
        }
        if (ptr.isNull())
        {
                return;
        }

        m_timer.stop();

        event->accept();
}

void PainterWindow::make_menu()
{
        m_show_threads_action = ui.menu_view->addAction("Show threads");
        m_show_threads_action->setCheckable(true);
        m_show_threads_action->setChecked(SHOW_THREADS);

        QObject::connect(
                ui.menu_window->addAction("Adjust size"), &QAction::triggered, this,
                &PainterWindow::adjust_window_size);
}

void PainterWindow::init_interface(const std::string& name)
{
        std::string title = std::string(settings::APPLICATION_NAME);
        if (!name.empty())
        {
                title += " - ";
                title += name;
        }
        this->setWindowTitle(QString::fromStdString(title));

        ui.main_widget->layout()->setContentsMargins(5, 5, 5, 5);

        ui.status_bar->setFixedHeight(ui.status_bar->height());

        ui.label_points->setText("");
        ui.label_points->resize(m_image_2d.width(), m_image_2d.height());

        ui.scrollAreaWidgetContents->layout()->setContentsMargins(0, 0, 0, 0);
        ui.scrollAreaWidgetContents->layout()->setSpacing(0);

        m_statistics_widget = std::make_unique<StatisticsWidget>(m_pixels.get(), UPDATE_INTERVAL_MILLISECONDS);
        ui.main_widget->layout()->addWidget(m_statistics_widget.get());

        connect(&m_timer, &QTimer::timeout, this, &PainterWindow::on_timer_timeout);
}

void PainterWindow::create_sliders(const std::vector<int>& screen_size)
{
        const int count = static_cast<int>(screen_size.size()) - 2;
        if (count <= 0)
        {
                return;
        }

        m_sliders_widget = std::make_unique<SlidersWidget>(screen_size);

        ASSERT(qobject_cast<QVBoxLayout*>(ui.main_widget->layout()));
        qobject_cast<QVBoxLayout*>(ui.main_widget->layout())->insertWidget(1, m_sliders_widget.get());

        connect(m_sliders_widget.get(), &SlidersWidget::changed, this,
                [this](const std::vector<int>& positions)
                {
                        m_pixels->set_slice_offset(positions);
                });

        m_sliders_widget->set(std::vector<int>(count, 0));
}

void PainterWindow::showEvent(QShowEvent* /*event*/)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        if (!m_first_show)
        {
                return;
        }
        m_first_show = false;

        QTimer::singleShot(50, this, &PainterWindow::on_first_shown);
}

void PainterWindow::on_first_shown()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        adjust_window_size();

        m_timer.start(UPDATE_INTERVAL_MILLISECONDS);
}

void PainterWindow::adjust_window_size()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        ui.scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui.scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        resize(QSize(2, 2) + m_image_2d.size() + (geometry().size() - ui.scroll_area->size()));

        ui.scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        ui.scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void PainterWindow::update_points()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        static_assert(std::endian::native == std::endian::little);
        static_assert(sizeof(quint32) == 4 * sizeof(std::byte));

        const std::span<const std::byte> pixels = m_pixels->slice();
        quint32* const image_bits = reinterpret_cast<quint32*>(m_image_2d.bits());

        ASSERT(data_size(pixels) == static_cast<size_t>(m_image_2d_byte_count));
        std::memcpy(image_bits, pixels.data(), m_image_2d_byte_count);

        if (m_show_threads_action->isChecked())
        {
                for (long long index : m_pixels->busy_indices_2d())
                {
                        if (index >= 0)
                        {
                                ASSERT(index < m_image_2d_pixel_count);
                                image_bits[index] ^= 0x00ff'ffff;
                        }
                }
        }

        ui.label_points->setPixmap(QPixmap::fromImage(m_image_2d));
        ui.label_points->update();
}

void PainterWindow::on_timer_timeout()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        m_statistics_widget->update();
        update_points();
        m_actions->set_progresses();
}
}
