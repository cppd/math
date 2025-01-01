/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <QMouseEvent>
#include <QObject>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QWidget>

namespace ns::gui::main_window
{
class GraphicsWidget final : public QWidget
{
        Q_OBJECT

public:
        explicit GraphicsWidget(QWidget* const parent)
                : QWidget(parent)
        {
                // setMouseTracking(true);
        }

Q_SIGNALS:
        void mouse_move(QMouseEvent* e);
        void mouse_press(QMouseEvent* e);
        void mouse_release(QMouseEvent* e);
        void mouse_wheel(QWheelEvent* e);
        void widget_resize(QResizeEvent* e);

protected:
        void wheelEvent(QWheelEvent* const e) override
        {
                Q_EMIT mouse_wheel(e);
        }

        void mouseMoveEvent(QMouseEvent* const e) override
        {
                Q_EMIT mouse_move(e);
        }

        void mousePressEvent(QMouseEvent* const e) override
        {
                Q_EMIT mouse_press(e);
        }

        void mouseReleaseEvent(QMouseEvent* const e) override
        {
                Q_EMIT mouse_release(e);
        }

        void resizeEvent(QResizeEvent* const e) override
        {
                Q_EMIT widget_resize(e);
        }
};
}
