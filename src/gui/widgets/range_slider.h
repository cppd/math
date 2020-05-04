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

#include "ui_range_slider.h"

class RangeSlider final : public QWidget
{
        Q_OBJECT

public:
        explicit RangeSlider(QWidget* parent = nullptr);

        void set_range(double min, double max);

signals:
        void range_changed(double min, double max);

private slots:
        void min_value_changed(int);
        void max_value_changed(int);

private:
        int min_value() const;
        int max_value() const;
        void set_min_value(int);
        void set_max_value(int);

        void emit_range_changed();

        double m_last_min;
        double m_last_max;

        Ui::RangeSlider ui;
};
