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

#include <QLabel>
#include <QResizeEvent>
#include <QWheelEvent>

class GraphicsWidget : public QLabel
{
        Q_OBJECT

public:
        explicit GraphicsWidget(QWidget* parent = nullptr, Qt::WindowFlags = Qt::WindowFlags()) : QLabel(parent)
        {
        }

        ~GraphicsWidget() override
        {
        }

signals:
        void wheel(double delta);
        void resize();

protected:
        // Это нужно для Винды для перехвата сообщений о вращении
        // колеса мыши над окном с графикой, встроенным дочерним
        // окном. В Линуксе может работать и без этого.
        void wheelEvent(QWheelEvent* event) override
        {
                QPoint local_mouse_pos = this->mapFromGlobal(event->globalPos());

                if (this->rect().contains(local_mouse_pos))
                {
                        emit wheel(event->angleDelta().ry() / 120.0);
                }
        }

        void resizeEvent(QResizeEvent*) override
        {
                emit resize();
        }
};
