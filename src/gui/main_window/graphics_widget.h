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

#include <QMouseEvent>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QWidget>

namespace gui
{
class GraphicsWidget final : public QWidget
{
        Q_OBJECT

public:
        explicit GraphicsWidget(QWidget* parent = nullptr, Qt::WindowFlags = Qt::WindowFlags()) : QWidget(parent)
        {
                // setMouseTracking(true);
        }

signals:
        void mouse_move(QMouseEvent* e);
        void mouse_press(QMouseEvent* e);
        void mouse_release(QMouseEvent* e);
        void mouse_wheel(QWheelEvent* e);
        void widget_resize(QResizeEvent* e);

protected:
        void wheelEvent(QWheelEvent* e) override
        {
                emit mouse_wheel(e);
        }

        void mouseMoveEvent(QMouseEvent* e) override
        {
                emit mouse_move(e);
        }

        void mousePressEvent(QMouseEvent* e) override
        {
                emit mouse_press(e);
        }

        void mouseReleaseEvent(QMouseEvent* e) override
        {
                emit mouse_release(e);
        }

        void resizeEvent(QResizeEvent* e) override
        {
                emit widget_resize(e);
        }
};
}
