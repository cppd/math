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

#include "ui_path_tracing_parameters.h"

class PathTracingParameters final : public QDialog
{
        Q_OBJECT

public:
        explicit PathTracingParameters(QWidget* parent = nullptr);
        [[nodiscard]] bool show(int max_thread_count, int width, int height, int* thread_count, double* size_coef);

private slots:
        void on_doubleSpinBox_image_size_valueChanged(double);

private:
        int m_width;
        int m_height;
        int m_max_thread_count;

        int m_thread_count;
        double m_size_coef;

        Ui::PathTracingParameters ui;

        void done(int r) override;
};
