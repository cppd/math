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

#include "painter_window_2d.h"

#include "../dialogs/message.h"

#include <src/com/error.h>
#include <src/com/exception.h>
#include <src/com/print.h>
#include <src/settings/name.h>

#include <QCloseEvent>
#include <QPointer>
#include <cmath>
#include <cstring>

namespace gui::painter_window_implementation
{
namespace
{
constexpr int UPDATE_INTERVAL_MILLISECONDS = 100;
// Этот интервал должен быть больше интервала UPDATE_INTERVAL_MILLISECONDS
constexpr int DIFFERENCE_INTERVAL_MILLISECONDS = 10 * UPDATE_INTERVAL_MILLISECONDS;
static_assert(DIFFERENCE_INTERVAL_MILLISECONDS > UPDATE_INTERVAL_MILLISECONDS);
constexpr bool SHOW_THREADS = true;

//

void set_label_minimum_width_for_text(QLabel* label, const std::string& text)
{
        label->setMinimumWidth(label->fontMetrics().boundingRect(text.c_str()).width());
}

void set_text_and_minimum_width(QLabel* label, const std::string& text)
{
        label->setText(text.c_str());
        label->setMinimumWidth(std::max(label->width(), label->fontMetrics().boundingRect(text.c_str()).width()));
}

std::string action_name(const QObject* action)
{
        ASSERT(qobject_cast<const QAction*>(action));

        std::string s = qobject_cast<const QAction*>(action)->text().toStdString();
        while (!s.empty() && s.back() == '.')
        {
                s.pop_back();
        }
        return s;
}

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

struct PainterWindow2d::Counters final
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

PainterWindow2d::PainterWindow2d(
        const std::string& name,
        const std::vector<int>& screen_size,
        const std::vector<int>& initial_slider_positions)
        : m_screen_size(screen_size),
          m_width(m_screen_size[0]),
          m_height(m_screen_size[1]),
          m_image_pixel_count(static_cast<long long>(m_width) * m_height),
          m_image_byte_count(static_cast<long long>(m_width) * m_height * sizeof(quint32)),
          m_image(m_width, m_height, QImage::Format_RGB32),
          m_difference(std::make_unique<Difference<Counters>>(DIFFERENCE_INTERVAL_MILLISECONDS))
{
        ASSERT(m_image.sizeInBytes() == m_image_byte_count);

        ui.setupUi(this);

        make_menu();
        init_interface(name);
        make_sliders(initial_slider_positions);
}

PainterWindow2d::~PainterWindow2d() = default;

void PainterWindow2d::closeEvent(QCloseEvent* event)
{
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

        event->accept();
}

void PainterWindow2d::make_menu()
{
        QObject::connect(ui.menu_actions->addAction("Save..."), &QAction::triggered, this, &PainterWindow2d::on_save);

        if (m_screen_size.size() == 3)
        {
                QObject::connect(
                        ui.menu_actions->addAction("Save all..."), &QAction::triggered, this,
                        &PainterWindow2d::on_save_all_with_background);

                QObject::connect(
                        ui.menu_actions->addAction("Save all without background..."), &QAction::triggered, this,
                        &PainterWindow2d::on_save_all_without_background);

                ui.menu_actions->addSeparator();

                QObject::connect(
                        ui.menu_actions->addAction("Add volume"), &QAction::triggered, this,
                        &PainterWindow2d::on_add_volume_with_background);

                QObject::connect(
                        ui.menu_actions->addAction("Add volume without background"), &QAction::triggered, this,
                        &PainterWindow2d::on_add_volume_without_background);
        }

        ui.menu_actions->addSeparator();

        QObject::connect(ui.menu_actions->addAction("Close..."), &QAction::triggered, this, &PainterWindow2d::close);

        //

        m_show_threads = ui.menu_view->addAction("Show threads");
        m_show_threads->setCheckable(true);
        m_show_threads->setChecked(SHOW_THREADS);

        //

        QObject::connect(
                ui.menu_window->addAction("Adjust size"), &QAction::triggered, this,
                &PainterWindow2d::adjust_window_size);
}

void PainterWindow2d::init_interface(const std::string& name)
{
        std::string title = std::string(settings::APPLICATION_NAME);
        if (!name.empty())
        {
                title += " - ";
                title += name;
        }
        this->setWindowTitle(QString::fromStdString(title));

        ui.status_bar->setFixedHeight(ui.status_bar->height());

        ui.label_points->setText("");
        ui.label_points->resize(m_width, m_height);

        ui.label_rays_per_second->setText("");
        ui.label_ray_count->setText("");
        ui.label_pass_count->setText("");
        ui.label_samples_per_pixel->setText("");

        ui.scrollAreaWidgetContents->layout()->setContentsMargins(0, 0, 0, 0);
        ui.scrollAreaWidgetContents->layout()->setSpacing(0);

        connect(&m_timer, &QTimer::timeout, this, &PainterWindow2d::on_timer_timeout);
}

void PainterWindow2d::make_sliders(const std::vector<int>& initial_slider_positions)
{
        const int slider_count = static_cast<int>(m_screen_size.size()) - 2;

        ASSERT(static_cast<int>(initial_slider_positions.size()) == slider_count);

        if (slider_count <= 0)
        {
                return;
        }

        m_dimension_sliders.resize(slider_count);

        QWidget* layout_widget = new QWidget(this);
        ui.main_widget->layout()->addWidget(layout_widget);

        QGridLayout* layout = new QGridLayout(layout_widget);
        layout_widget->setLayout(layout);
        layout->setContentsMargins(0, 0, 0, 0);

        for (int i = 0; i < slider_count; ++i)
        {
                int dimension = i + 2;
                int dimension_max_value = m_screen_size[dimension] - 1;

                m_dimension_sliders[i].slider.setOrientation(Qt::Horizontal);
                m_dimension_sliders[i].slider.setMinimum(0);
                m_dimension_sliders[i].slider.setMaximum(dimension_max_value);

                ASSERT(initial_slider_positions[i] >= 0 && initial_slider_positions[i] <= dimension_max_value);
                m_dimension_sliders[i].slider.setValue(initial_slider_positions[i]);

                set_label_minimum_width_for_text(
                        &m_dimension_sliders[i].label, to_string_digit_groups(dimension_max_value));
                m_dimension_sliders[i].label.setText(to_string_digit_groups(initial_slider_positions[i]).c_str());

                QLabel* label_d = new QLabel(QString("d[") + to_string(dimension + 1).c_str() + "]", layout_widget);
                QLabel* label_e = new QLabel("=", layout_widget);

                layout->addWidget(label_d, i, 0);
                layout->addWidget(label_e, i, 1);
                layout->addWidget(&m_dimension_sliders[i].label, i, 2);
                layout->addWidget(&m_dimension_sliders[i].slider, i, 3);

                connect(&m_dimension_sliders[i].slider, &QSlider::valueChanged, this,
                        &PainterWindow2d::on_slider_changed);
        }
}

std::vector<int> PainterWindow2d::slider_positions() const
{
        std::vector<int> positions;
        positions.reserve(m_dimension_sliders.size());
        for (const DimensionSlider& s : m_dimension_sliders)
        {
                positions.push_back(s.slider.value());
        }
        return positions;
}

void PainterWindow2d::showEvent(QShowEvent* /*event*/)
{
        if (!m_first_show)
        {
                return;
        }
        m_first_show = false;

        QTimer::singleShot(50, this, &PainterWindow2d::on_first_shown);
}

void PainterWindow2d::on_first_shown()
{
        adjust_window_size();

        m_timer.start(UPDATE_INTERVAL_MILLISECONDS);
}

void PainterWindow2d::adjust_window_size()
{
        ui.scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui.scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        resize(QSize(2 + m_width, 2 + m_height) + (geometry().size() - ui.scroll_area->size()));

        ui.scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        ui.scroll_area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

void PainterWindow2d::update_statistics()
{
        Statistics statistics = this->statistics();

        auto [difference, duration] =
                m_difference->compute(Counters(statistics.pixel_count, statistics.ray_count, statistics.sample_count));

        long long rays_per_second =
                (duration != 0) ? std::llround(static_cast<double>(difference.ray_count) / duration) : 0;

        long long samples_per_pixel =
                (difference.pixel_count != 0)
                        ? std::llround(static_cast<double>(difference.sample_count) / difference.pixel_count)
                        : 0;

        long long milliseconds_per_frame = std::llround(1000 * statistics.previous_pass_duration);

        set_text_and_minimum_width(ui.label_rays_per_second, to_string_digit_groups(rays_per_second));
        set_text_and_minimum_width(ui.label_ray_count, to_string_digit_groups(statistics.ray_count));
        set_text_and_minimum_width(
                ui.label_pass_count, to_string_digit_groups(statistics.pass_number)
                                             .append(progress_to_string(":", statistics.pass_progress)));
        set_text_and_minimum_width(ui.label_samples_per_pixel, to_string_digit_groups(samples_per_pixel));
        set_text_and_minimum_width(ui.label_milliseconds_per_frame, to_string_digit_groups(milliseconds_per_frame));
}

void PainterWindow2d::update_points()
{
        static_assert(std::is_same_v<quint32, std::uint_least32_t>);

        quint32* const image_bits = reinterpret_cast<quint32*>(m_image.bits());
        const long long offset = pixels_offset();
        std::memcpy(image_bits, &pixels_bgra32()[offset], m_image_byte_count);
        if (m_show_threads->isChecked())
        {
                for (long long index : busy_indices_2d())
                {
                        if (index >= 0)
                        {
                                ASSERT(index < m_image_pixel_count);
                                image_bits[index] ^= 0x00ff'ffff;
                        }
                }
        }
        ui.label_points->setPixmap(QPixmap::fromImage(m_image));
        ui.label_points->update();
}

void PainterWindow2d::on_timer_timeout()
{
        update_statistics();
        update_points();
}

void PainterWindow2d::on_slider_changed(int)
{
        ASSERT(qobject_cast<const QSlider*>(sender()));

        const QSlider* const slider = qobject_cast<const QSlider*>(sender());
        for (DimensionSlider& dm : m_dimension_sliders)
        {
                if (&dm.slider == slider)
                {
                        set_text_and_minimum_width(&dm.label, to_string_digit_groups(dm.slider.value()));
                        slider_positions_change_event(slider_positions());
                        return;
                }
        }
        error_fatal("Failed to find sender in sliders");
}

void PainterWindow2d::on_save()
{
        catch_all(
                action_name(sender()),
                [this]()
                {
                        save_to_file();
                });
}

void PainterWindow2d::on_save_all_with_background()
{
        catch_all(
                action_name(sender()),
                [this]()
                {
                        save_all_to_files(false);
                });
}

void PainterWindow2d::on_save_all_without_background()
{
        catch_all(
                action_name(sender()),
                [this]()
                {
                        save_all_to_files(true);
                });
}

void PainterWindow2d::on_add_volume_with_background()
{
        catch_all(
                action_name(sender()),
                [this]()
                {
                        add_volume(false);
                });
}

void PainterWindow2d::on_add_volume_without_background()
{
        catch_all(
                action_name(sender()),
                [this]()
                {
                        add_volume(true);
                });
}
}
