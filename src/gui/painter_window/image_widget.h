/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "ui_image_widget.h"

#include <QImage>
#include <QMenu>

#include <cstddef>
#include <span>
#include <vector>

namespace ns::gui::painter_window
{
class ImageWidget final : public QWidget
{
        Q_OBJECT

private:
        Ui::ImageWidget ui_;

        const long long image_2d_pixel_count_;
        const std::size_t image_2d_byte_count_;
        QImage image_2d_;

        QAction* show_threads_action_ = nullptr;

public:
        ImageWidget(int width, int height, QMenu* menu);

        [[nodiscard]] QSize size_difference() const;

        void update(std::span<const std::byte> pixels_r8g8b8a8, const std::vector<long long>& busy_indices);
};
}
