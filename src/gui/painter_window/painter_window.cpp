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
#include <src/com/exception.h>
#include <src/com/print.h>
#include <src/settings/name.h>

#include <QCloseEvent>
#include <QPointer>
#include <cmath>
#include <cstring>

namespace gui::painter_window
{
namespace
{
constexpr int UPDATE_INTERVAL_MILLISECONDS = 100;
// Этот интервал должен быть больше интервала UPDATE_INTERVAL_MILLISECONDS
constexpr int DIFFERENCE_INTERVAL_MILLISECONDS = 10 * UPDATE_INTERVAL_MILLISECONDS;
static_assert(DIFFERENCE_INTERVAL_MILLISECONDS > UPDATE_INTERVAL_MILLISECONDS);
constexpr bool SHOW_THREADS = true;

std::string progress_to_string(const char* prefix, double progress)
{
        int percent = std::floor(progress * 100.0);
        percent = (percent < 100) ? percent : 99;
        std::string progress_str = prefix;
        progress_str += percent < 10 ? "0" : "";
        progress_str += std::to_string(percent);
        return progress_str;
}
}

struct PainterWindow::Counters final
{
        const long long pixel_count;
        const long long ray_count;
        const long long sample_count;

        Counters(long long pixel_count, long long ray_count, long long sample_count)
                : pixel_count(pixel_count), ray_count(ray_count), sample_count(sample_count)
        {
        }

        Counters operator-(const Counters& c) const
        {
                return Counters(pixel_count - c.pixel_count, ray_count - c.ray_count, sample_count - c.sample_count);
        }
};

PainterWindow::PainterWindow(const std::string& name, std::unique_ptr<Pixels>&& pixels)
        : m_image_2d_pixel_count(1ull * pixels->screen_size()[0] * pixels->screen_size()[1]),
          m_image_2d_byte_count(sizeof(quint32) * m_image_2d_pixel_count),
          m_image_2d(pixels->screen_size()[0], pixels->screen_size()[1], QImage::Format_RGB32),
          m_difference(std::make_unique<Difference<Counters>>(DIFFERENCE_INTERVAL_MILLISECONDS)),
          m_pixels(std::move(pixels)),
          m_pixel_count(multiply_all<long long>(m_pixels->screen_size()))
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

        ui.label_rays_per_second->setText("");
        ui.label_ray_count->setText("");
        ui.label_pass_count->setText("");
        ui.label_samples_per_pixel->setText("");
        ui.label_milliseconds_per_frame->setText("");

        ui.scrollAreaWidgetContents->layout()->setContentsMargins(0, 0, 0, 0);
        ui.scrollAreaWidgetContents->layout()->setSpacing(0);

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

void PainterWindow::update_statistics()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        const painter::Statistics s = m_pixels->statistics();

        auto [difference, duration] = m_difference->compute(Counters(s.pixel_count, s.ray_count, s.sample_count));

        long long rays_per_second =
                (duration != 0) ? std::llround(static_cast<double>(difference.ray_count) / duration) : 0;

        long long samples_per_pixel =
                (difference.pixel_count != 0)
                        ? std::llround(static_cast<double>(difference.sample_count) / difference.pixel_count)
                        : 0;

        long long milliseconds_per_frame = std::llround(1000 * s.previous_pass_duration);

        double pass_progress = static_cast<double>(s.pass_pixel_count) / m_pixel_count;

        set_text_and_minimum_width(ui.label_rays_per_second, to_string_digit_groups(rays_per_second));
        set_text_and_minimum_width(ui.label_ray_count, to_string_digit_groups(s.ray_count));
        set_text_and_minimum_width(
                ui.label_pass_count,
                to_string_digit_groups(s.pass_number).append(progress_to_string(":", pass_progress)));
        set_text_and_minimum_width(ui.label_samples_per_pixel, to_string_digit_groups(samples_per_pixel));
        set_text_and_minimum_width(ui.label_milliseconds_per_frame, to_string_digit_groups(milliseconds_per_frame));
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

        update_statistics();
        update_points();
        m_actions->set_progresses();
}
}
