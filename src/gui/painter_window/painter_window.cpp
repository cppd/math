/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/com/error.h>
#include <src/settings/name.h>

#include <QCloseEvent>
#include <QPointer>

namespace ns::gui::painter_window
{
namespace
{
constexpr std::chrono::milliseconds UPDATE_INTERVAL{200};
constexpr std::chrono::milliseconds WINDOW_SHOW_DELAY{50};
}

PainterWindow::PainterWindow(const std::string& name, std::unique_ptr<Pixels>&& pixels) : m_pixels(std::move(pixels))
{
        ui.setupUi(this);

        std::string title = std::string(settings::APPLICATION_NAME);
        if (!name.empty())
        {
                title += " - ";
                title += name;
        }
        this->setWindowTitle(QString::fromStdString(title));

        create_interface();
        create_sliders();
        create_actions();
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
        std::optional<bool> yes = dialog::message_question_default_no("Do you want to close the painter window?");
        if (ptr.isNull())
        {
                return;
        }
        if (!yes || !*yes)
        {
                event->ignore();
                return;
        }

        m_timer.stop();

        event->accept();
}

void PainterWindow::create_interface()
{
        ui.status_bar->setFixedHeight(ui.status_bar->height());

        ASSERT(!ui.main_widget->layout());
        ui.main_widget->setLayout(new QVBoxLayout(ui.main_widget));
        ui.main_widget->layout()->setContentsMargins(0, 0, 0, 0);
        ui.main_widget->layout()->setSpacing(0);

        m_image_widget =
                std::make_unique<ImageWidget>(m_pixels->screen_size()[0], m_pixels->screen_size()[1], ui.menu_view);
        ui.main_widget->layout()->addWidget(m_image_widget.get());

        m_statistics_widget = std::make_unique<StatisticsWidget>(UPDATE_INTERVAL);
        ui.main_widget->layout()->addWidget(m_statistics_widget.get());

        ui.status_bar->addPermanentWidget(new QLabel(m_pixels->color_name(), this));
        ui.status_bar->addPermanentWidget(new QLabel(m_pixels->floating_point_name(), this));

        connect(ui.menu_window->addAction("Adjust size"), &QAction::triggered, this,
                &PainterWindow::adjust_window_size);

        connect(&m_timer, &QTimer::timeout, this, &PainterWindow::on_timer_timeout);
}

void PainterWindow::create_sliders()
{
        const int count = static_cast<int>(m_pixels->screen_size().size()) - 2;
        if (count <= 0)
        {
                m_slice = 0;
                return;
        }

        m_sliders_widget = std::make_unique<SlidersWidget>(m_pixels->screen_size());

        ASSERT(qobject_cast<QVBoxLayout*>(ui.main_widget->layout()));
        qobject_cast<QVBoxLayout*>(ui.main_widget->layout())->insertWidget(1, m_sliders_widget.get());

        connect(m_sliders_widget.get(), &SlidersWidget::changed, this,
                [this](const std::vector<int>& positions)
                {
                        ASSERT(!positions.empty() && positions.size() + 2 == m_pixels->screen_size().size());
                        // ((x[3]*size[2] + x[2])*size[1] + x[1])*size[0] + x[0]
                        long long slice = 0;
                        for (int i = static_cast<int>(positions.size()) - 1; i >= 0; --i)
                        {
                                std::size_t dimension = i + 2;
                                ASSERT(dimension < m_pixels->screen_size().size());
                                ASSERT(positions[i] >= 0 && positions[i] < m_pixels->screen_size()[dimension]);
                                slice = slice * m_pixels->screen_size()[dimension] + positions[i];
                        }
                        m_slice = slice;
                });
        m_sliders_widget->set(std::vector<int>(count, 0));
}

void PainterWindow::create_actions()
{
        QMenu* menu = ui.menu_actions;

        m_actions = std::make_unique<Actions>(
                m_pixels.get(), menu, ui.status_bar,
                [this]
                {
                        return m_slice;
                });

        if (!menu->actions().empty())
        {
                menu->addSeparator();
        }
        connect(menu->addAction("Exit..."), &QAction::triggered, this, &PainterWindow::close);
}

void PainterWindow::showEvent(QShowEvent* /*event*/)
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        if (!m_first_show)
        {
                return;
        }
        m_first_show = false;

        QTimer::singleShot(WINDOW_SHOW_DELAY, this, &PainterWindow::on_first_shown);
}

void PainterWindow::on_first_shown()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        adjust_window_size();

        m_timer.start(UPDATE_INTERVAL);
}

void PainterWindow::adjust_window_size()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        resize(QSize(2, 2) + geometry().size() + m_image_widget->size_difference());
}

void PainterWindow::on_timer_timeout()
{
        ASSERT(std::this_thread::get_id() == m_thread_id);

        m_statistics_widget->update(m_pixels->statistics());
        m_image_widget->update(m_pixels->slice_r8g8b8a8(m_slice), m_pixels->busy_indices_2d());
        m_actions->set_progresses();
}
}
