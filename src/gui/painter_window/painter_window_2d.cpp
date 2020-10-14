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
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/time.h>
#include <src/settings/name.h>

#include <QCloseEvent>
#include <QPointer>
#include <array>
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
}

class PainterWindow2d::Difference
{
        struct Point
        {
                std::array<long long, 3> data;
                TimePoint time;
                Point(const std::array<long long, 3>& data, const TimePoint& time) : data(data), time(time)
                {
                }
        };

        const TimeDuration m_interval;
        std::deque<Point> m_deque;

public:
        explicit Difference(int interval_milliseconds) : m_interval(std::chrono::milliseconds(interval_milliseconds))
        {
        }

        std::tuple<long long, long long, long long, double> compute(const std::array<long long, 3>& data)
        {
                TimePoint now = time();
                TimePoint p = now - m_interval;

                // Удаление старых элементов
                while (!m_deque.empty() && m_deque.front().time < p)
                {
                        m_deque.pop_front();
                }

                m_deque.emplace_back(data, now);

                return std::make_tuple(
                        m_deque.back().data[0] - m_deque.front().data[0],
                        m_deque.back().data[1] - m_deque.front().data[1],
                        m_deque.back().data[2] - m_deque.front().data[2],
                        duration(m_deque.front().time, m_deque.back().time));
        }
};

PainterWindow2d::PainterWindow2d(
        const std::string& name,
        std::vector<int>&& screen_size,
        const std::vector<int>& initial_slider_positions)
        : m_screen_size(std::move(screen_size)),
          m_width(m_screen_size[0]),
          m_height(m_screen_size[1]),
          m_image_pixel_count(static_cast<long long>(m_width) * m_height),
          m_image_byte_count(static_cast<long long>(m_width) * m_height * sizeof(quint32)),
          m_image(m_width, m_height, QImage::Format_RGB32),
          m_difference(std::make_unique<Difference>(DIFFERENCE_INTERVAL_MILLISECONDS))
{
        ui.setupUi(this);

        std::string title = std::string(settings::APPLICATION_NAME);
        if (!name.empty())
        {
                title += " - ";
                title += name;
        }
        this->setWindowTitle(QString::fromStdString(title));

        connect(ui.pushButton_save_to_file, &QPushButton::clicked, this, &PainterWindow2d::on_save_to_file_clicked);
        connect(ui.pushButton_add_volume, &QPushButton::clicked, this, &PainterWindow2d::on_add_volume_clicked);

        connect(&m_timer, &QTimer::timeout, this, &PainterWindow2d::on_timer_timeout);

        ASSERT(m_image.sizeInBytes() == m_image_byte_count);

        init_interface(initial_slider_positions);
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

void PainterWindow2d::init_interface(const std::vector<int>& initial_slider_positions)
{
        ui.pushButton_add_volume->setVisible(m_screen_size.size() == 3);

        ui.label_points->setText("");
        ui.label_points->resize(m_width, m_height);

        ui.label_rays_per_second->setText("");
        ui.label_ray_count->setText("");
        ui.label_pass_count->setText("");
        ui.label_samples_per_pixel->setText("");

        ui.scrollAreaWidgetContents->layout()->setContentsMargins(0, 0, 0, 0);
        ui.scrollAreaWidgetContents->layout()->setSpacing(0);
        this->layout()->setContentsMargins(5, 5, 5, 5);

        ui.checkBox_show_threads->setChecked(SHOW_THREADS);

        const int slider_count = static_cast<int>(m_screen_size.size()) - 2;

        ASSERT(static_cast<int>(initial_slider_positions.size()) == slider_count);

        if (slider_count <= 0)
        {
                return;
        }

        m_dimension_sliders.resize(slider_count);

        QWidget* layout_widget = new QWidget(this);
        this->layout()->addWidget(layout_widget);

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
        std::vector<int> positions(m_dimension_sliders.size());

        for (unsigned i = 0; i < m_dimension_sliders.size(); ++i)
        {
                positions[i] = m_dimension_sliders[i].slider.value();
        }

        return positions;
}

void PainterWindow2d::showEvent(QShowEvent* e)
{
        QWidget::showEvent(e);

        if (!m_first_show)
        {
                return;
        }
        m_first_show = false;

        QTimer::singleShot(50, this, &PainterWindow2d::on_first_shown);
}

void PainterWindow2d::on_first_shown()
{
        ui.scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui.scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        resize(QSize(2 + m_width, 2 + m_height) + (geometry().size() - ui.scrollArea->size()));

        ui.scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        ui.scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        m_timer.start(UPDATE_INTERVAL_MILLISECONDS);
}

void PainterWindow2d::update_statistics()
{
        long long pass_count;
        long long pixel_count;
        long long ray_count;
        long long sample_count;
        double previous_pass_duration;

        painter_statistics(&pass_count, &pixel_count, &ray_count, &sample_count, &previous_pass_duration);

        auto [ray_diff, sample_diff, pixel_diff, time_diff] =
                m_difference->compute({ray_count, sample_count, pixel_count});

        long long rays_per_second = time_diff != 0 ? std::llround(ray_diff / time_diff) : 0;
        long long samples_per_pixel = pixel_diff != 0 ? std::llround(static_cast<double>(sample_diff) / pixel_diff) : 0;

        long long milliseconds_per_frame = std::llround(1000 * previous_pass_duration);

        set_text_and_minimum_width(ui.label_rays_per_second, to_string_digit_groups(rays_per_second));
        set_text_and_minimum_width(ui.label_ray_count, to_string_digit_groups(ray_count));
        set_text_and_minimum_width(ui.label_pass_count, to_string_digit_groups(pass_count));
        set_text_and_minimum_width(ui.label_samples_per_pixel, to_string_digit_groups(samples_per_pixel));
        set_text_and_minimum_width(ui.label_milliseconds_per_frame, to_string_digit_groups(milliseconds_per_frame));
}

void PainterWindow2d::update_points()
{
        static_assert(std::is_same_v<quint32, std::uint_least32_t>);

        long long offset = pixels_offset();
        std::memcpy(m_image.bits(), &pixels_bgr()[offset], m_image_byte_count);
        if (ui.checkBox_show_threads->isChecked())
        {
                for (long long index : pixels_busy())
                {
                        long long index_in_image = index - offset;
                        if (index_in_image >= 0 && index_in_image < m_image_pixel_count)
                        {
                                reinterpret_cast<quint32*>(m_image.bits())[index_in_image] ^= 0x00ff'ffff;
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

void PainterWindow2d::on_save_to_file_clicked()
{
        catch_all("Save to file", [&]() { save_to_file(); });
}

void PainterWindow2d::on_add_volume_clicked()
{
        catch_all("Volume", [&]() { add_volume(); });
}

void PainterWindow2d::on_slider_changed(int)
{
        QObject* s = sender();
        for (DimensionSlider& dm : m_dimension_sliders)
        {
                if (&dm.slider == s)
                {
                        set_text_and_minimum_width(&dm.label, to_string_digit_groups(dm.slider.value()));
                        slider_positions_change_event(slider_positions());
                        return;
                }
        }
        error_fatal("Failed to find sender in sliders");
}
}
