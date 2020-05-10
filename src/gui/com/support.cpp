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

#include "support.h"

#include <src/color/color.h>
#include <src/com/error.h>
#include <src/com/log.h>

#include <QApplication>
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

void append_plain_text(QPlainTextEdit* text_edit, const std::vector<std::string>& lines)
{
        if (lines.empty())
        {
                return;
        }
        QString s;
        s += QString::fromStdString(lines.front());
        for (auto iter = std::next(lines.cbegin()); iter != lines.cend(); ++iter)
        {
                s += '\n';
                s += QString::fromStdString(*iter);
        }
        text_edit->appendPlainText(s);
}

void append_html(
        QPlainTextEdit* text_edit,
        const QString& line_begin,
        const QString& line_end,
        const std::vector<std::string>& lines)
{
        if (lines.empty())
        {
                return;
        }
        QString s;
        s += line_begin;
        s += QString::fromStdString(lines.front()).toHtmlEscaped();
        s += line_end;
        for (auto iter = std::next(lines.cbegin()); iter != lines.cend(); ++iter)
        {
                s += '\n';
                s += line_begin;
                s += QString::fromStdString(*iter).toHtmlEscaped();
                s += line_end;
        }
        text_edit->appendHtml(s);
}

void write_to_text_edit(QPlainTextEdit* text_edit, const std::vector<std::string>& lines, TextEditMessageType type)
{
        ASSERT(text_edit);

        // text_edit->moveCursor(QTextCursor::End);

        switch (type)
        {
        case TextEditMessageType::Normal:
        {
                append_plain_text(text_edit, lines);
                break;
        }
        case TextEditMessageType::Error:
        {
                const QString& begin = QStringLiteral(R"(<pre><font color="Red">)");
                const QString& end = QStringLiteral(R"(</font></pre>)");
                append_html(text_edit, begin, end, lines);
                break;
        }
        case TextEditMessageType::Warning:
        {
                const QString& begin = QStringLiteral(R"(<pre><font color="#d08000">)");
                const QString& end = QStringLiteral(R"(</font></pre>)");
                append_html(text_edit, begin, end, lines);
                break;
        }
        case TextEditMessageType::Information:
        {
                const QString& begin = QStringLiteral(R"(<pre><font color="Blue">)");
                const QString& end = QStringLiteral(R"(</font></pre>)");
                append_html(text_edit, begin, end, lines);
                break;
        }
        }
}
}

QWidget* parent_for_dialog()
{
        return qApp->activeWindow();
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
        return Color(Srgb8(r, g, b));
}

void button_strike_out(QRadioButton* button, bool strike_out)
{
        ASSERT(button);

        QFont f = button->font();
        f.setStrikeOut(strike_out);
        button->setFont(f);
}

void set_slider_to_middle(QSlider* slider)
{
        slider->setValue(slider->minimum() + (slider->maximum() - slider->minimum()) / 2);
}

void add_to_text_edit_and_to_stderr(
        QPlainTextEdit* text_edit,
        const std::vector<std::string>& lines,
        TextEditMessageType type) noexcept
{
        try
        {
                ASSERT(text_edit);

                try
                {
                        if (lines.empty())
                        {
                                return;
                        }

                        write_formatted_log_messages_to_stderr(lines);

                        bool bottom =
                                text_edit->verticalScrollBar()->value() == text_edit->verticalScrollBar()->maximum()
                                || text_edit->verticalScrollBar()->maximum() == 0;

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
                catch (const std::exception& e)
                {
                        error_fatal(std::string("error adding log message: ") + e.what());
                }
        }
        catch (...)
        {
                error_fatal("error adding log message");
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

WindowID widget_window_id(const QWidget* widget)
{
        ASSERT(widget);

        static_assert(sizeof(WindowID) == sizeof(WId));
        static_assert(std::is_integral_v<WindowID> || std::is_pointer_v<WindowID>);
        static_assert(std::is_integral_v<WId> || std::is_pointer_v<WId>);

        union
        {
                WId w_id;
                WindowID window_id;
        };

        w_id = widget->winId();
        return window_id;
}

double widget_pixels_per_inch(const QWidget* widget)
{
#if 0
        int n = QApplication::desktop()->screenNumber(widget);
        QScreen* s = QApplication::screens().at(n);
        return s->logicalDotsPerInch();
#else
        double ppi_x = widget->logicalDpiX();
        double ppi_y = widget->logicalDpiY();
        double ppi = 0.5 * (ppi_x + ppi_y);
        ASSERT(ppi > 0);
        return ppi;
#endif
}

void move_window_to_desktop_center(QMainWindow* window)
{
        ASSERT(window);

        // Из документации на move: the position on the desktop, including frame
        window->move(
                (QDesktopWidget().availableGeometry(window).width() - window->frameGeometry().width()) / 2,
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

double slider_position(const QSlider* slider)
{
        double v = slider->value();
        double min = slider->minimum();
        double max = slider->maximum();
        ASSERT(max > min);
        return (v - min) / (max - min);
}

void set_slider_position(QSlider* slider, double v)
{
        double min = slider->minimum();
        double max = slider->maximum();
        ASSERT(max > min);
        slider->setValue(std::lround(std::lerp(min, max, std::clamp(v, 0.0, 1.0))));
}
