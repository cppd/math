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

#pragma once

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/type/detect.h>
#include <src/numerical/vec.h>
#include <src/window/handle.h>

#include <QLayout>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QRadioButton>
#include <QSlider>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

template <typename Window, typename... Args>
void create_and_show_delete_on_close_window(Args&&... args)
{
        std::unique_ptr<Window> window = std::make_unique<Window>(std::forward<Args>(args)...);
        window->show();
        window->setAttribute(Qt::WA_DeleteOnClose);
        static_cast<void>(window.release());
}

template <typename Window, typename... Args>
Window* create_delete_on_close_window(Args&&... args)
{
        std::unique_ptr<Window> window = std::make_unique<Window>(std::forward<Args>(args)...);
        window->setAttribute(Qt::WA_DeleteOnClose);
        return window.release();
}

// Чтобы объект Qt, имеющий родителя, не удалялся два и более раз, нужно использовать
// динамическую память и класс QPointer.
// Такие возможные многократные удаления могут происходить, например, когда родительское
// окно диалогового окна удаляется во время выполнения функции exec диалогового окна,
// и тогда после функции exec уже нельзя удалять диалоговое окно.
template <typename T>
class QtObjectInDynamicMemory final : public QPointer<T>
{
public:
        template <typename... Args>
        explicit QtObjectInDynamicMemory(Args&&... args) : QPointer<T>(new T(std::forward<Args>(args)...))
        {
        }

        ~QtObjectInDynamicMemory()
        {
                delete *static_cast<QPointer<T>*>(this);
        }
};

QWidget* parent_for_dialog();

void set_widgets_enabled(QLayout* layout, bool v);

Color qcolor_to_rgb(const QColor& c);

void button_strike_out(QRadioButton* button, bool strike_out);

void set_slider_to_middle(QSlider* slider);

void add_to_text_edit(QPlainTextEdit* text_edit, const std::vector<std::string>& lines, LogMessageType type) noexcept;

WindowID widget_window_id(const QWidget* widget);
double widget_pixels_per_inch(const QWidget* widget);

void move_window_to_desktop_center(QMainWindow* window);

void resize_window_frame(QMainWindow* window, const QSize& frame_size);
void resize_window_widget(QMainWindow* window, QWidget* widget, const QSize& widget_size);

double slider_position(const QSlider* slider);
void set_slider_position(QSlider* slider, double v);
