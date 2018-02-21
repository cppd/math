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

#include "support.h"

#include "com/color/colors.h"
#include "com/error.h"
#include "com/log.h"

#include <QButtonGroup>
#include <QDesktopWidget>
#include <QScrollBar>
#include <type_traits>
#include <utility>

namespace
{
bool is_child_widget_of_any_layout(QLayout* layout, QWidget* widget)
{
        if (layout == nullptr || widget == nullptr)
        {
                return false;
        }
        if (layout->indexOf(widget) >= 0)
        {
                return true;
        }
        foreach (QObject* o, layout->children())
        {
                if (is_child_widget_of_any_layout(static_cast<QLayout*>(o), widget))
                {
                        return true;
                }
        }
        return false;
}

void write_to_text_edit(QTextEdit* text_edit, const std::vector<std::string>& lines, TextEditMessageType type)
{
        ASSERT(text_edit);

        const char* line_begin;
        const char* line_end;

        switch (type)
        {
        case TextEditMessageType::Normal:
                line_begin = "";
                line_end = "<br>";
                break;
        case TextEditMessageType::Error:
                line_begin = "<font color=\"Red\">";
                line_end = "</font><br>";
                break;
        case TextEditMessageType::Warning:
                line_begin = "<font color=\"#d08000\">";
                line_end = "</font><br>";
                break;
        case TextEditMessageType::Information:
                line_begin = "<font color=\"Blue\">";
                line_end = "</font><br>";
                break;
        }

        text_edit->moveCursor(QTextCursor::End);

        for (const std::string& s : lines)
        {
                text_edit->insertHtml(line_begin + QString(s.c_str()).toHtmlEscaped() + line_end);
        }
}
}

void set_widgets_enabled(QLayout* layout, bool v)
{
        ASSERT(layout);

        QWidget* pw = layout->parentWidget();
        if (!pw)
        {
                return;
        }
        foreach (QWidget* w, pw->findChildren<QWidget*>())
        {
                if (is_child_widget_of_any_layout(layout, w))
                {
                        w->setEnabled(v);
                }
        }
}

Color qcolor_to_rgb(const QColor& c)
{
        unsigned char r = std::clamp(c.red(), 0, 255);
        unsigned char g = std::clamp(c.green(), 0, 255);
        unsigned char b = std::clamp(c.blue(), 0, 255);
        return Color(SrgbInteger(r, g, b));
}

void button_strike_out(QRadioButton* button, bool strike_out)
{
        ASSERT(button);

        QFont f = button->font();
        f.setStrikeOut(strike_out);
        button->setFont(f);
}

void add_to_text_edit_and_to_stderr(QTextEdit* text_edit, const std::vector<std::string>& lines,
                                    TextEditMessageType type) noexcept
{
        ASSERT(text_edit);

        try
        {
                write_formatted_log_messages_to_stderr(lines);

                bool bottom = text_edit->verticalScrollBar()->value() == text_edit->verticalScrollBar()->maximum() ||
                              text_edit->verticalScrollBar()->maximum() == 0;

                if (bottom)
                {
                        write_to_text_edit(text_edit, lines, type);
                        text_edit->verticalScrollBar()->setValue(text_edit->verticalScrollBar()->maximum());
                }
                else
                {
                        int v = text_edit->verticalScrollBar()->value();
                        write_to_text_edit(text_edit, lines, type);
                        text_edit->verticalScrollBar()->setValue(v);
                }
        }
        catch (std::exception& e)
        {
                error_fatal(std::string("error add message: ") + e.what());
        }
        catch (...)
        {
                error_fatal("error add message");
        }
}

#if 0
void disable_radio_button(QRadioButton* button)
{
        ASSERT(button && button->group());

        button->group()->setExclusive(false);
        // button->setAutoExclusive(false);
        button->setChecked(false);
        // button->setAutoExclusive(true);
        button->setEnabled(false);
        button->group()->setExclusive(true);
}
#endif

WindowID widget_window_id(QWidget* widget)
{
        ASSERT(widget);

        static_assert(sizeof(WindowID) == sizeof(WId));
        static_assert(std::is_integral_v<WindowID> || std::is_pointer_v<WindowID>);
        static_assert(std::is_integral_v<WId> || std::is_pointer_v<WId>);

        union {
                WId w_id;
                WindowID window_id;
        };

        w_id = widget->winId();
        return window_id;
}

void move_window_to_desktop_center(QMainWindow* window)
{
        ASSERT(window);

        // Из документации на move: the position on the desktop, including frame
        window->move((QDesktopWidget().availableGeometry(window).width() - window->frameGeometry().width()) / 2,
                     (QDesktopWidget().availableGeometry(window).height() - window->frameGeometry().height()) / 2);
}

// Изменение размеров окна
void resize_window_frame(QMainWindow* window, const QSize& frame_size)
{
        ASSERT(window);

        // Из документации на resize: the size excluding any window frame
        window->resize(frame_size - (window->frameGeometry().size() - window->geometry().size()));
}

// Изменение размеров окна для получения заданного размера заданного объекта этого окна
void resize_window_widget(QMainWindow* window, QWidget* widget, const QSize& widget_size)
{
        ASSERT(window && widget);

        // Из документации на resize: the size excluding any window frame
        window->resize(widget_size + (window->geometry().size() - widget->size()));
}
