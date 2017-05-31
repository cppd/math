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
#ifndef PENCIL_H
#define PENCIL_H

#include "gl/gl_objects.h"

#include <memory>

class PencilEffect final
{
        class Impl;
        std::unique_ptr<Impl> m_impl;

public:
        PencilEffect(const Texture2D& tex, const TextureR32I& tex_objects);
        ~PencilEffect();

        void draw();
};

#endif
