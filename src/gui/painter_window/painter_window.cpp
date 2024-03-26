/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "actions.h"
#include "image_widget.h"
#include "pixels.h"
#include "sliders_widget.h"
#include "statistics_widget.h"

#include "ui_painter_window.h"

#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/gui/com/support.h>
#include <src/gui/dialogs/message.h>
#include <src/painter/painter.h>
#include <src/settings/name.h>

#include <QAction>
#include <QCloseEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QObject>
#include <QPointer>
#include <QShowEvent>
#include <QSize>
#include <QSlider>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <Qt>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace ns::gui::painter_window
{
namespace
{
constexpr std::chrono::milliseconds UPDATE_INTERVAL{200};
constexpr std::chrono::milliseconds WINDOW_SHOW_DELAY{50};

const char* integrator_to_string(const painter::Integrator integrator)
{
        switch (integrator)
        {
        case painter::Integrator::BPT:
                return "BPT";
        case painter::Integrator::PT:
                return "PT";
        }
        error("Unknown painter integrator " + to_string(enum_to_int(integrator)));
}
}

PainterWindow::PainterWindow(
        const std::string& name,
        const painter::Integrator integrator,
        const char* const floating_point_name,
        const char* const color_name,
        std::unique_ptr<Pixels>&& pixels)
        : pixels_(std::move(pixels))
{
        ui_.setupUi(this);

        std::string title = settings::APPLICATION_NAME;
        if (!name.empty())
        {
                title += " - ";
                title += name;
        }
        this->setWindowTitle(QString::fromStdString(title));

        create_interface(integrator_to_string(integrator), floating_point_name, color_name);
        create_sliders();
        create_actions();
}

PainterWindow::~PainterWindow()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        timer_.stop();

        actions_.reset();
        pixels_.reset();
}

void PainterWindow::closeEvent(QCloseEvent* const event)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        const QPointer ptr(this);
        const std::optional<bool> yes = dialog::message_question_default_no("Do you want to close the painter window?");
        if (ptr.isNull())
        {
                return;
        }
        if (!yes || !*yes)
        {
                event->ignore();
                return;
        }

        timer_.stop();

        event->accept();
}

void PainterWindow::create_interface(
        const char* const integrator_name,
        const char* const floating_point_name,
        const char* const color_name)
{
        ui_.status_bar->setFixedHeight(ui_.status_bar->height());

        ASSERT(!ui_.main_widget->layout());
        auto* const main_layout = new QVBoxLayout(ui_.main_widget);
        main_layout->setContentsMargins(0, 0, 0, 0);
        main_layout->setSpacing(0);

        auto* const image_widget = new QWidget(this);
        auto* const image_layout = new QHBoxLayout(image_widget);
        image_layout->setContentsMargins(0, 0, 0, 0);
        image_layout->setSpacing(0);
        main_layout->addWidget(image_widget);

        brightness_parameter_slider_ = std::make_unique<QSlider>(Qt::Vertical);
        brightness_parameter_slider_->setTracking(false);
        brightness_parameter_slider_->setValue(0);
        pixels_->set_brightness_parameter(0);
        connect(brightness_parameter_slider_.get(), &QSlider::valueChanged, this,
                [this](int)
                {
                        pixels_->set_brightness_parameter(
                                std::clamp(slider_position(brightness_parameter_slider_.get()), 0.0, 1.0));
                });
        image_layout->addWidget(brightness_parameter_slider_.get());

        image_widget_ =
                std::make_unique<ImageWidget>(pixels_->screen_size()[0], pixels_->screen_size()[1], ui_.menu_view);
        image_layout->addWidget(image_widget_.get());

        statistics_widget_ = std::make_unique<StatisticsWidget>(UPDATE_INTERVAL);
        main_layout->addWidget(statistics_widget_.get());

        ui_.status_bar->addPermanentWidget(new QLabel(integrator_name, this));
        ui_.status_bar->addPermanentWidget(new QLabel(color_name, this));
        ui_.status_bar->addPermanentWidget(new QLabel(floating_point_name, this));

        connect(ui_.menu_window->addAction("Adjust size"), &QAction::triggered, this,
                &PainterWindow::adjust_window_size);

        connect(&timer_, &QTimer::timeout, this, &PainterWindow::update_image);
}

void PainterWindow::create_sliders()
{
        const int count = static_cast<int>(pixels_->screen_size().size()) - 2;
        if (count <= 0)
        {
                slice_ = 0;
                return;
        }

        sliders_widget_ = std::make_unique<SlidersWidget>(pixels_->screen_size());

        QVBoxLayout* const layout = qobject_cast<QVBoxLayout*>(ui_.main_widget->layout());
        ASSERT(layout);
        layout->insertWidget(1, sliders_widget_.get());

        auto widget_changed = [this](const std::vector<int>& positions)
        {
                static constexpr int POSITION_TO_DIMENSION = 2;
                const std::vector<int>& screen_size = pixels_->screen_size();
                ASSERT(!positions.empty() && positions.size() + POSITION_TO_DIMENSION == screen_size.size());
                // ((x[3]*size[2] + x[2])*size[1] + x[1])*size[0] + x[0]
                long long slice = 0;
                for (int i = static_cast<int>(positions.size()) - 1; i >= 0; --i)
                {
                        const std::size_t dimension = i + POSITION_TO_DIMENSION;
                        ASSERT(positions[i] >= 0 && positions[i] < screen_size[dimension]);
                        slice = slice * screen_size[dimension] + positions[i];
                }
                slice_ = slice;
        };

        connect(sliders_widget_.get(), &SlidersWidget::changed, this, std::move(widget_changed));

        sliders_widget_->set(std::vector(count, 0));
}

void PainterWindow::create_actions()
{
        QMenu* const menu = ui_.menu_actions;

        actions_ = std::make_unique<Actions>(
                pixels_.get(), menu, ui_.status_bar,
                [this]
                {
                        return slice_;
                });

        if (!menu->actions().empty())
        {
                menu->addSeparator();
        }
        connect(menu->addAction("Exit..."), &QAction::triggered, this, &PainterWindow::close);
}

void PainterWindow::showEvent(QShowEvent* const /*event*/)
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        if (!first_show_)
        {
                return;
        }
        first_show_ = false;

        QTimer::singleShot(WINDOW_SHOW_DELAY, this, &PainterWindow::on_first_shown);
}

void PainterWindow::on_first_shown()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        adjust_window_size();

        timer_.start(UPDATE_INTERVAL);
}

void PainterWindow::adjust_window_size()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        resize(QSize(2, 2) + geometry().size() + image_widget_->size_difference());
}

void PainterWindow::update_image()
{
        ASSERT(std::this_thread::get_id() == thread_id_);

        statistics_widget_->update(pixels_->statistics(), pixels_->pixel_max());
        image_widget_->update(pixels_->slice_r8g8b8a8(slice_), pixels_->busy_indices_2d());
        actions_->set_progresses();
}
}
