/*
Copyright (C) 2017 Topological Manifold

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

#include "com/vec.h"
#include "window/window_prop.h"

#include <QColorDialog>
#include <QLayout>
#include <QMainWindow>
#include <QRadioButton>
#include <QString>
#include <QTextEdit>
#include <string>
#include <vector>

enum class TextEditMessageType
{
        Normal,
        Error,
        Warning,
        Information
};

template <typename T>
void color_dialog(QWidget* widget, const QString& title, const QColor& color, const T& f)
{
        QColorDialog dialog(widget);

        dialog.setCurrentColor(color);
        dialog.setWindowTitle(title);
        dialog.setOptions(QColorDialog::NoButtons | QColorDialog::DontUseNativeDialog);

        widget->connect(&dialog, &QColorDialog::currentColorChanged, [&f](const QColor& c) {
                if (c.isValid())
                {
                        f(c);
                }
        });

        dialog.exec();
}

void set_widgets_enabled(QLayout* layout, bool v);

vec3 qcolor_to_rgb(const QColor& c);

void button_strike_out(QRadioButton* button, bool strike_out);

void add_to_text_edit_and_to_stderr(QTextEdit* text_edit, const std::vector<std::string>& lines,
                                    TextEditMessageType type) noexcept;

WindowID get_widget_window_id(QWidget* widget);

void move_window_to_desktop_center(QMainWindow* window);

void resize_window_frame(QMainWindow* window, const QSize& frame_size);
void resize_window_widget(QMainWindow* window, QWidget* widget, const QSize& widget_size);
