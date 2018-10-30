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

#include "canvas.h"

namespace
{
class Canvas final : public VulkanCanvas
{
        void set_text_color(const Color& /*c*/) override
        {
        }

        void set_text_active(bool /*v*/) override
        {
        }

        void set_pencil_effect_active(bool /*v*/) override
        {
        }

        bool pencil_effect_active() override
        {
                return false;
        }

        void set_dft_active(bool /*v*/) override
        {
        }

        bool dft_active() override
        {
                return false;
        }

        void set_dft_brightness(double /*v*/) override
        {
        }

        void set_dft_background_color(const Color& /*c*/) override
        {
        }

        void set_dft_color(const Color& /*c*/) override
        {
        }

        void set_convex_hull_active(bool /*v*/) override
        {
        }

        void set_optical_flow_active(bool /*v*/) override
        {
        }
};
}

std::unique_ptr<VulkanCanvas> create_vulkan_canvas()
{
        return std::make_unique<Canvas>();
}
