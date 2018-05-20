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

#include "painter_window_2d.h"

#include "com/error.h"
#include "com/file/file_sys.h"
#include "com/log.h"
#include "com/print.h"
#include "com/time.h"
#include "ui/dialogs/messages/file_dialog.h"
#include "ui/dialogs/messages/message_box.h"
#include "ui/support/support.h"

#include <QCloseEvent>
#include <QPointer>
#include <array>
#include <cmath>
#include <cstring>

constexpr int UPDATE_INTERVAL_MILLISECONDS = 100;
// Этот интервал должен быть больше интервала UPDATE_INTERVAL_MILLISECONDS
constexpr int DIFFERENCE_INTERVAL_MILLISECONDS = 10 * UPDATE_INTERVAL_MILLISECONDS;

constexpr bool SHOW_THREADS = true;

constexpr const char IMAGE_FILE_FORMAT[] = "png";

namespace
{
void set_label_minimum_width_for_text(QLabel* label, const std::string& text)
{
        label->setMinimumWidth(label->fontMetrics().width(text.c_str()));
}

void set_text_and_minimum_width(QLabel* label, const std::string& text)
{
        label->setText(text.c_str());
        label->setMinimumWidth(std::max(label->width(), label->fontMetrics().width(text.c_str())));
}
}

class PainterWindow2d::Difference
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
                double time = time_in_seconds();

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

PainterWindow2d::PainterWindow2d(const std::string& title, std::vector<int>&& screen_size,
                                 const std::vector<int>& initial_slider_positions)
        : m_window_thread_id(std::this_thread::get_id()),
          m_screen_size(std::move(screen_size)),
          m_width(m_screen_size[0]),
          m_height(m_screen_size[1]),
          m_image(m_width, m_height, QImage::Format_RGB32),
          m_image_byte_count(m_width * m_height * sizeof(quint32)),
          m_first_show(true),
          m_difference(std::make_unique<Difference>(DIFFERENCE_INTERVAL_MILLISECONDS))
{
        ui.setupUi(this);

        this->setWindowTitle(title.c_str());

        connect(this, SIGNAL(error_message_signal(QString)), this, SLOT(error_message_slot(QString)),
                Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

        connect(&m_timer, SIGNAL(timeout()), this, SLOT(timer_slot()));

        ASSERT(m_image.byteCount() == m_image_byte_count);

        init_interface(initial_slider_positions);
}

PainterWindow2d::~PainterWindow2d() = default;

template <typename F>
void PainterWindow2d::catch_all(const F& function) const noexcept
{
        try
        {
                ASSERT(std::this_thread::get_id() == m_window_thread_id);

                std::string msg;
                QPointer ptr(this);
                try
                {
                        function(&msg);
                }
                catch (std::exception& e)
                {
                        std::string s = !msg.empty() ? (msg + ":\n") : std::string();
                        if (!ptr.isNull())
                        {
                                error_message(s + e.what());
                        }
                        else
                        {
                                LOG("Exception caught.\n" + s + e.what());
                        }
                }
                catch (...)
                {
                        std::string s = !msg.empty() ? (msg + ":\n") : std::string();
                        if (!ptr.isNull())
                        {
                                error_message(s + "Unknown error");
                        }
                        else
                        {
                                LOG("Exception caught.\n" + s + "Unknow Error");
                        }
                }
        }
        catch (...)
        {
                error_fatal("Exception in the painter window 2d catch all");
        }
}

void PainterWindow2d::closeEvent(QCloseEvent* event)
{
        QPointer ptr(this);

        if (!message_question_default_no(this, "Do you want to close the painter window?"))
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

                set_label_minimum_width_for_text(&m_dimension_sliders[i].label, to_string_digit_groups(dimension_max_value));
                m_dimension_sliders[i].label.setText(to_string_digit_groups(initial_slider_positions[i]).c_str());

                QLabel* label_d = new QLabel(QString("d[") + to_string(dimension + 1).c_str() + "]", layout_widget);
                QLabel* label_e = new QLabel("=", layout_widget);

                layout->addWidget(label_d, i, 0);
                layout->addWidget(label_e, i, 1);
                layout->addWidget(&m_dimension_sliders[i].label, i, 2);
                layout->addWidget(&m_dimension_sliders[i].slider, i, 3);

                connect(&m_dimension_sliders[i].slider, SIGNAL(valueChanged(int)), this, SLOT(slider_changed_slot(int)));
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

void PainterWindow2d::error_message(const std::string& msg) const noexcept
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

void PainterWindow2d::error_message_slot(QString msg)
{
        LOG("Painter error\n" + msg.toStdString());

        message_critical(this, msg);
}

void PainterWindow2d::showEvent(QShowEvent* e)
{
        QWidget::showEvent(e);

        if (!m_first_show)
        {
                return;
        }
        m_first_show = false;

        QTimer::singleShot(50, this, SLOT(first_shown()));
}

void PainterWindow2d::first_shown()
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
        long long pass_count, pixel_count, ray_count, sample_count;
        double previous_pass_duration;

        painter_statistics(&pass_count, &pixel_count, &ray_count, &sample_count, &previous_pass_duration);

        auto [ray_diff, sample_diff, pixel_diff, time_diff] = m_difference->compute({{ray_count, sample_count, pixel_count}});

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
        std::memcpy(m_image.bits(), pixel_pointer(ui.checkBox_show_threads->isChecked()), m_image_byte_count);
        ui.label_points->setPixmap(QPixmap::fromImage(m_image));
        ui.label_points->update();
}

void PainterWindow2d::timer_slot()
{
        update_statistics();
        update_points();
}

void PainterWindow2d::on_pushButton_save_to_file_clicked()
{
        catch_all([&](std::string* msg) {
                *msg = "Save to file";

                QImage image(m_image.width(), m_image.height(), m_image.format());
                std::memcpy(image.bits(), pixel_pointer(false), m_image_byte_count);

                std::string file_name;

                std::string caption = "Save";
                std::string filter = file_filter("Images", IMAGE_FILE_FORMAT);
                bool read_only = true;

                QPointer ptr(this);
                if (!save_file_name(this, caption, filter, read_only, &file_name))
                {
                        return;
                }
                if (ptr.isNull())
                {
                        return;
                }

                std::string ext = file_extension(file_name);
                if (ext.size() > 0)
                {
                        if (ext != IMAGE_FILE_FORMAT)
                        {
                                error("Unsupported image file format " + ext);
                        }
                }
                else
                {
                        file_name += "."; // может оказаться 2 точки подряд
                        file_name += IMAGE_FILE_FORMAT;
                }

                if (!image.save(file_name.c_str()))
                {
                        error("Error saving image to file " + file_name);
                }
        });
}

void PainterWindow2d::slider_changed_slot(int)
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
