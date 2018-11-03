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

#include "graphics/api.h"

#include "ui_gc_api_selection.h"

#include <string>

namespace graphics_and_compute_api_selection_implementation
{
class GraphicsAndComputeAPISelection final : public QDialog
{
        Q_OBJECT

public:
        explicit GraphicsAndComputeAPISelection(QWidget* parent);
        [[nodiscard]] bool show(GraphicsAndComputeAPI* api);

private:
        Ui::GraphicsAndComputeAPISelection ui;

        GraphicsAndComputeAPI m_api;

        void done(int r) override;
};
}

namespace dialog
{
[[nodiscard]] bool graphics_and_compute_api_selection(QWidget* parent, GraphicsAndComputeAPI* api);
}
