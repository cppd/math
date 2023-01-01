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

#include <src/numerical/vector.h>

#include <span>

namespace ns::image
{
void blend_alpha_r8g8b8a8(const std::span<std::byte>& bytes, const Vector<3, float>& rgb);

void blend_alpha_r8g8b8a8_premultiplied(const std::span<std::byte>& bytes, const Vector<3, float>& rgb);

void blend_alpha_r16g16b16a16(const std::span<std::byte>& bytes, const Vector<3, float>& rgb);

void blend_alpha_r16g16b16a16_srgb(const std::span<std::byte>& bytes, const Vector<3, float>& rgb);

void blend_alpha_r16g16b16a16_premultiplied(const std::span<std::byte>& bytes, const Vector<3, float>& rgb);

void blend_alpha_r32g32b32a32(const std::span<std::byte>& bytes, const Vector<3, float>& rgb);

void blend_alpha_r32g32b32a32_premultiplied(const std::span<std::byte>& bytes, const Vector<3, float>& rgb);
}
