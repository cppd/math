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

#include "pixels.h"

#include "ui_image_widget.h"

#include <QImage>
#include <QMenu>

namespace ns::gui::painter_window
{
class ImageWidget final : public QWidget
{
        Q_OBJECT

private:
        Ui::ImageWidget ui;

        const Pixels* m_pixels;

        const long long m_image_2d_pixel_count;
        const std::size_t m_image_2d_byte_count;
        QImage m_image_2d;

        QAction* m_show_threads_action = nullptr;

public:
        ImageWidget(const Pixels* pixels, QMenu* menu);

        QSize size_difference() const;

        void update();
};
}
