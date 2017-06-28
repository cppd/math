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

#include <QLabel>
#include <QObject>
#include <QWheelEvent>

#include "com/log.h"
#include "com/print.h"

// Это нужно для Винды для перехвата сообщений о вращении колеса мыши над окном с графикой,
// встроенным дочерним окном. В Линуксе может работать и без этого.

class WheelLabel : public QLabel
{
        Q_OBJECT

public:
        explicit WheelLabel(QWidget* parent = nullptr, Qt::WindowFlags = Qt::WindowFlags()) : QLabel(parent)
        {
        }
        ~WheelLabel()
        {
        }

signals:
        void wheel(double delta);

protected:
        void wheelEvent(QWheelEvent* event) override
        {
                QPoint local_mouse_pos = this->mapFromGlobal(event->globalPos());

                QRect label_rect(0, 0, this->width(), this->height());

                if (label_rect.contains(local_mouse_pos))
                {
                        emit wheel(event->angleDelta().ry() / 120.0);
                }
        }
};
