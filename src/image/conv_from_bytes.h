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

#include <cstddef>
#include <span>
#include <vector>

namespace ns::image::conv
{
void r8_srgb_to_r32(const std::span<const std::byte>& bytes, std::vector<float>* floats);

void r16_to_r32(const std::span<const std::byte>& bytes, std::vector<float>* floats);

void r8g8b8_srgb_to_r32g32b32(const std::span<const std::byte>& bytes, std::vector<float>* floats);

void r16g16b16_to_r32g32b32(const std::span<const std::byte>& bytes, std::vector<float>* floats);

void r16g16b16_srgb_to_r32g32b32(const std::span<const std::byte>& bytes, std::vector<float>* floats);

void r8g8b8a8_srgb_to_r32g32b32a32(const std::span<const std::byte>& bytes, std::vector<float>* floats);

void r16g16b16a16_to_r32g32b32a32(const std::span<const std::byte>& bytes, std::vector<float>* floats);

void r16g16b16a16_srgb_to_r32g32b32a32(const std::span<const std::byte>& bytes, std::vector<float>* floats);

void copy(const std::span<const std::byte>& bytes, std::vector<float>* floats);
}
