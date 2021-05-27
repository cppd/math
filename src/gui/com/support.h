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

#pragma once

#include <src/color/color.h>
#include <src/window/handle.h>

#include <QLabel>
#include <QLayout>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPointer>
#include <QRadioButton>
#include <QSlider>
#include <QSplitter>
#include <string>

namespace ns::gui
{
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

        explicit QtObjectInDynamicMemory(T* ptr) : QPointer<T>(ptr)
        {
        }

        ~QtObjectInDynamicMemory()
        {
                delete this->data();
        }
};

template <typename Window, typename... Args>
void create_and_show_delete_on_close_window(Args&&... args)
{
        QtObjectInDynamicMemory<Window> window(std::forward<Args>(args)...);
        window->show();
        window->setAttribute(Qt::WA_DeleteOnClose);
        window.clear();
}

template <typename Window, typename... Args>
Window* create_delete_on_close_window(Args&&... args)
{
        QtObjectInDynamicMemory<Window> window(std::forward<Args>(args)...);
        window->setAttribute(Qt::WA_DeleteOnClose);
        Window* ptr = window;
        window.clear();
        return ptr;
}

QWidget* parent_for_dialog();

// std::string main_window_title();

// void set_widgets_enabled(QLayout* layout, bool v);

QSplitter* find_widget_splitter(QObject* object, QWidget* widget);

void set_horizontal_stretch(QWidget* widget, int stretchFactor);

Color qcolor_to_rgb(const QColor& c);
QColor rgb_to_qcolor(const Color& c);

void set_widget_color(QWidget* widget, const QColor& c);
void set_widget_color(QWidget* widget, const Color& c);

// void button_strike_out(QRadioButton* button, bool strike_out);

void set_slider_to_middle(QSlider* slider);

void append_to_text_edit(QPlainTextEdit* text_edit, const std::string& text, const RGB8& color) noexcept;

window::WindowID widget_window_id(const QWidget* widget);
double widget_pixels_per_inch(const QWidget* widget);

void move_window_to_desktop_center(QMainWindow* window);

void resize_window_frame(QMainWindow* window, const QSize& frame_size);
void resize_window_widget(QMainWindow* window, QWidget* widget, const QSize& widget_size);

double slider_position(const QSlider* slider);
void set_slider_position(QSlider* slider, double v);

void add_widget(QWidget* dst, QWidget* src);

void set_label_minimum_width_for_text(QLabel* label, const std::string& text);
void set_label_text_and_minimum_width(QLabel* label, const std::string& text);
}
