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

#include "graphics/opengl/objects.h"

#include <memory>

class PencilEffect final
{
        class Impl;
        std::unique_ptr<Impl> m_impl;

public:
        PencilEffect(const opengl::TextureRGBA32F& source, bool source_is_srgb, const opengl::TextureR32I& objects,
                     const mat4& matrix);
        ~PencilEffect();

        void draw();
};
