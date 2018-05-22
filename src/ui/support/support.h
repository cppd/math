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

#pragma once

#include "com/color/colors.h"
#include "com/error.h"
#include "com/type_detect.h"
#include "com/vec.h"
#include "window/window_prop.h"

#include <QLayout>
#include <QMainWindow>
#include <QPointer>
#include <QRadioButton>
#include <QSlider>
#include <QTextEdit>
#include <iterator>
#include <string>
#include <type_traits>
#include <vector>

template <typename Window, typename... Args>
void create_and_show_delete_on_close_window(Args&&... args)
{
        Window* window = new Window(std::forward<Args>(args)...);

        try
        {
                window->show();
                window->setAttribute(Qt::WA_DeleteOnClose);
        }
        catch (...)
        {
                delete window;
                throw;
        }
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
        QtObjectInDynamicMemory(Args&&... args) : QPointer<T>(new T(std::forward<Args>(args)...))
        {
        }

        ~QtObjectInDynamicMemory()
        {
                delete *static_cast<QPointer<T>*>(this);
        }
};

template <typename... T>
std::string file_filter(const std::string& name, const T&... extensions)
{
        static_assert(sizeof...(T) > 0);

        if (name.empty())
        {
                error("No filter file name");
        }

        std::string filter;

        filter += name + " (";

        bool first = true;

        auto add_string = [&](const std::string& ext) {
                if (std::count(ext.cbegin(), ext.cend(), '*') > 0)
                {
                        error("Character * in file filter extension " + ext);
                }
                if (!first)
                {
                        filter += " ";
                }
                first = false;
                filter += "*." + ext;
        };

        auto add = [&](const auto& ext) {
                if constexpr (has_begin_end<decltype(ext)>)
                {
                        if constexpr (!std::is_same_v<char,
                                                      std::remove_cv_t<std::remove_reference_t<decltype(*std::cbegin(ext))>>>)
                        {
                                for (const std::string& e : ext)
                                {
                                        add_string(e);
                                }
                        }
                        else
                        {
                                add_string(ext);
                        }
                }
                else
                {
                        add_string(ext);
                }
        };

        (add(extensions), ...);

        if (first)
        {
                error("No file filter extensions");
        }

        filter += ")";

        return filter;
}

enum class TextEditMessageType
{
        Normal,
        Error,
        Warning,
        Information
};

void set_widgets_enabled(QLayout* layout, bool v);

Color qcolor_to_rgb(const QColor& c);

void button_strike_out(QRadioButton* button, bool strike_out);

void set_slider_to_middle(QSlider* slider);

void add_to_text_edit_and_to_stderr(QTextEdit* text_edit, const std::vector<std::string>& lines,
                                    TextEditMessageType type) noexcept;

WindowID widget_window_id(QWidget* widget);

void move_window_to_desktop_center(QMainWindow* window);

void resize_window_frame(QMainWindow* window, const QSize& frame_size);
void resize_window_widget(QMainWindow* window, QWidget* widget, const QSize& widget_size);
