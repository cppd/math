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
#ifndef QT_SUPPORT_H
#define QT_SUPPORT_H

#include <QColor>
#include <QLayout>
#include <QRadioButton>
#include <glm/vec3.hpp>

void set_widgets_enabled(QLayout* layout, bool v);

glm::vec3 qcolor_to_vec3(const QColor& c);

void disable_radio_button(QRadioButton* button);

void button_strike_out(QRadioButton* button, bool strike_out);

#endif
