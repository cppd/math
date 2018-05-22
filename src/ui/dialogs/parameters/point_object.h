/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "ui_point_object.h"

#include <string>

namespace point_object_parameters_implementation
{
class PointObjectParameters final : public QDialog
{
        Q_OBJECT

public:
        explicit PointObjectParameters(QWidget* parent = nullptr);

        [[nodiscard]] bool show(int dimension, const std::string& point_object_name, int default_point_count, int min_point_count,
                                int max_point_count, int* point_count);

private:
        int m_min_point_count;
        int m_max_point_count;
        int m_point_count;

        Ui::PointObjectParameters ui;

        void done(int r) override;
};
}

namespace dialog
{
[[nodiscard]] bool point_object_parameters(QWidget* parent, int dimension, const std::string& point_object_name,
                                           int default_point_count, int min_point_count, int max_point_count, int* point_count);
}
