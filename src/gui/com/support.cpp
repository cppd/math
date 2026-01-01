/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "application.h"

#include <src/color/color.h>
#include <src/color/rgb8.h>
#include <src/com/conversion.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/window/handle.h>

#include <QApplication>
#include <QColor>
#include <QDialog>
#include <QLabel>
#include <QMainWindow>
#include <QObject>
#include <QPalette>
#include <QPlainTextEdit>
#include <QScreen>
#include <QScrollBar>
#include <QSizePolicy>
#include <QSlider>
#include <QSplitter>
#include <QString>
#include <QWidget>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <exception>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

namespace ns::gui::com
{
namespace
{
// bool is_child_widget_of_any_layout(QLayout* const layout, QWidget* const widget)
// {
//         if (!layout || !widget)
//         {
//                 return false;
//         }
//         if (layout->indexOf(widget) >= 0)
//         {
//                 return true;
//         }
//         for (QObject* const object : layout->children())
//         {
//                 if (is_child_widget_of_any_layout(qobject_cast<QLayout*>(object), widget))
//                 {
//                         return true;
//                 }
//         }
//         return false;
// }

QString to_qstring(const std::string_view text)
{
        return QString::fromUtf8(text.data(), text.size());
}

void append_text(QPlainTextEdit* const text_edit, const std::string_view text, const color::RGB8 color)
{
        // text_edit->moveCursor(QTextCursor::End);

        if (color == color::RGB8(0, 0, 0))
        {
                text_edit->appendPlainText(to_qstring(text));
        }
        else
        {
                QString s;
                s += QStringLiteral("<pre><font color=\"");
                s += QColor(color.red(), color.green(), color.blue()).name();
                s += QStringLiteral("\">");
                s += to_qstring(text).toHtmlEscaped();
                s += QStringLiteral(R"(</font></pre>)");
                text_edit->appendHtml(s);
        }
}
}

QWidget* parent_for_dialog()
{
        return Application::instance()->activeWindow();
}

// std::string main_window_title()
// {
//         QMainWindow* main_window = nullptr;
//         for (QWidget* const widget : Application::instance()->topLevelWidgets())
//         {
//                 QMainWindow* const window = qobject_cast<QMainWindow*>(widget);
//                 if (window)
//                 {
//                         ASSERT(!main_window);
//                         main_window = window;
//                 }
//         }
//         if (main_window)
//         {
//                 return main_window->windowTitle().toStdString();
//         }
//         return {};
// }

// void set_widgets_enabled(QLayout* const layout, const bool v)
// {
//         ASSERT(layout);
//
//         QWidget* const parent_widget = layout->parentWidget();
//         if (!parent_widget)
//         {
//                 return;
//         }
//         for (QWidget* const widget : parent_widget->findChildren<QWidget*>())
//         {
//                 if (is_child_widget_of_any_layout(layout, widget))
//                 {
//                         widget->setEnabled(v);
//                 }
//         }
// }

QSplitter* find_widget_splitter(QObject* const object, QWidget* const widget)
{
        auto* const splitter = qobject_cast<QSplitter*>(object);
        if (splitter && splitter->indexOf(widget) >= 0)
        {
                return splitter;
        }
        for (QObject* const child : object->children())
        {
                if (QSplitter* const s = find_widget_splitter(child, widget))
                {
                        return s;
                }
        }
        return nullptr;
}

void set_horizontal_stretch(QWidget* const widget, const int stretch_factor)
{
        QSizePolicy sp = widget->sizePolicy();
        sp.setHorizontalStretch(stretch_factor);
        widget->setSizePolicy(sp);
}

void set_vertical_stretch(QWidget* const widget, const int stretch_factor)
{
        QSizePolicy sp = widget->sizePolicy();
        sp.setVerticalStretch(stretch_factor);
        widget->setSizePolicy(sp);
}

color::Color qcolor_to_color(const QColor& c)
{
        const unsigned char r = std::clamp(c.red(), 0, 255);
        const unsigned char g = std::clamp(c.green(), 0, 255);
        const unsigned char b = std::clamp(c.blue(), 0, 255);
        return color::Color(color::RGB8(r, g, b));
}

QColor color_to_qcolor(const color::Color& c)
{
        const color::RGB8 srgb8 = color::make_rgb8(c.rgb32());
        return {srgb8.red(), srgb8.green(), srgb8.blue()};
}

void set_widget_color(QWidget* const widget, const QColor& c)
{
        ASSERT(widget);

        QPalette palette;
        palette.setColor(QPalette::Window, c);
        widget->setPalette(palette);
}

// void button_strike_out(QRadioButton* const button, const bool strike_out)
// {
//         ASSERT(button);
//
//         QFont f = button->font();
//         f.setStrikeOut(strike_out);
//         button->setFont(f);
// }

void set_slider_to_middle(QSlider* const slider)
{
        slider->setValue(slider->minimum() + (slider->maximum() - slider->minimum()) / 2);
}

void append_to_text_edit(QPlainTextEdit* const text_edit, const std::string_view text, const color::RGB8 color) noexcept
{
        try
        {
                ASSERT(text_edit);

                try
                {
                        if (text.empty())
                        {
                                return;
                        }

                        const bool bottom =
                                text_edit->verticalScrollBar()->value() == text_edit->verticalScrollBar()->maximum()
                                || text_edit->verticalScrollBar()->maximum() == 0;

                        if (bottom)
                        {
                                append_text(text_edit, text, color);
                                text_edit->verticalScrollBar()->setValue(text_edit->verticalScrollBar()->maximum());
                        }
                        else
                        {
                                const int v = text_edit->verticalScrollBar()->value();
                                append_text(text_edit, text, color);
                                text_edit->verticalScrollBar()->setValue(v);
                        }
                }
                catch (const std::exception& e)
                {
                        error_fatal(std::string("error adding to text edit: ") + e.what());
                }
        }
        catch (...)
        {
                error_fatal("error adding to text edit");
        }
}

// void disable_radio_button(QRadioButton* const button)
// {
//         ASSERT(button && button->group());
//         button->group()->setExclusive(false);
//         // button->setAutoExclusive(false);
//         button->setChecked(false);
//         // button->setAutoExclusive(true);
//         button->setEnabled(false);
//         button->group()->setExclusive(true);
// }

window::WindowID widget_window_id(const QWidget* const widget)
{
        ASSERT(widget);

        static_assert(sizeof(window::WindowID) == sizeof(decltype(widget->winId())));
        static_assert(std::is_integral_v<window::WindowID> || std::is_pointer_v<window::WindowID>);
        static_assert(std::is_integral_v<decltype(widget->winId())> || std::is_pointer_v<decltype(widget->winId())>);

        return std::bit_cast<window::WindowID>(widget->winId());
}

std::array<double, 2> widget_size(const QWidget* const widget)
{
        ASSERT(widget);

        const double width_mm = widget->widthMM();
        const double height_mm = widget->heightMM();

        const double width_pixels = widget->width();
        const double height_pixels = widget->height();
        const double logical_ppi = widget->logicalDpiX();
        const double width = pixels_to_millimeters(width_pixels, logical_ppi);
        const double height = pixels_to_millimeters(height_pixels, logical_ppi);
        const double rel_w = std::abs(width_mm - width) / std::max(std::abs(width_mm), std::abs(width));
        const double rel_h = std::abs(height_mm - height) / std::max(std::abs(height_mm), std::abs(height));

        if (!(rel_w < 0.01 && rel_h < 0.01))
        {
                std::ostringstream oss;
                oss << "Error finding widget size";
                oss << ": size mm = (" << width_mm << ", " << height_mm << ")";
                oss << "; size = (" << width << ", " << height << ")";
                oss << "; size pixels = (" << width_pixels << ", " << height_pixels << ")";
                oss << "; logical ppi = " << logical_ppi;
                oss << "; device pixel ratio = " << widget->devicePixelRatioF();
                oss << "; ppi = " << logical_ppi * widget->devicePixelRatioF();
                LOG(oss.str());
        }

        return {width_mm, height_mm};
}

void move_window_to_desktop_center(QMainWindow* const window)
{
        ASSERT(window);

        const QScreen& screen = *Application::instance()->primaryScreen();

        // move function includes frame
        window->move(
                (screen.availableGeometry().width() - window->frameGeometry().width()) / 2,
                (screen.availableGeometry().height() - window->frameGeometry().height()) / 2);
}

void resize_window_frame(QMainWindow* const window, const QSize& frame_size)
{
        ASSERT(window);

        // resize function excludes frame
        window->resize(frame_size - window->frameGeometry().size() + window->geometry().size());
}

void resize_window_widget(QMainWindow* const window, QWidget* const widget, const QSize& widget_size)
{
        ASSERT(window && widget);

        // resize function excludes frame
        window->resize(widget_size - widget->size() + window->geometry().size());
}

double slider_position(const QSlider* const slider)
{
        const double v = slider->value();
        const double min = slider->minimum();
        const double max = slider->maximum();
        ASSERT(max > min);
        return (v - min) / (max - min);
}

void set_slider_position(QSlider* const slider, const double v)
{
        const double min = slider->minimum();
        const double max = slider->maximum();
        ASSERT(max > min);
        slider->setValue(std::lround(std::lerp(min, max, std::clamp(v, 0.0, 1.0))));
}

void add_widget(QWidget* const dst, QWidget* const src)
{
        if (dst->layout())
        {
                error("Error setting widget: there already is a layout manager");
        }
        auto* const l = new QVBoxLayout(dst);
        l->setContentsMargins(0, 0, 0, 0);
        l->addWidget(src);
}

void set_label_minimum_width_for_text(QLabel* const label, const std::string_view text)
{
        label->setMinimumWidth(label->fontMetrics().boundingRect(to_qstring(text)).width());
}

void set_label_text_and_minimum_width(QLabel* const label, const std::string_view text)
{
        const QString s = to_qstring(text);
        label->setText(s);
        label->setMinimumWidth(std::max(label->width(), label->fontMetrics().boundingRect(s).width()));
}

void set_dialog_size(QDialog* const dialog)
{
        dialog->setMinimumSize(0, 0);
        dialog->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        dialog->adjustSize();
        const auto size = dialog->size();
        dialog->setMinimumSize(size);
        dialog->setMaximumSize(size);
}

void set_dialog_height(QDialog* const dialog)
{
        dialog->setMinimumHeight(0);
        dialog->setMaximumHeight(QWIDGETSIZE_MAX);
        dialog->adjustSize();
        const auto height = dialog->height();
        dialog->setMinimumHeight(height);
        dialog->setMaximumHeight(height);
}
}
