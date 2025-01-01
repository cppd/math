/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <src/com/alg.h>

#include <array>
#include <cstdint>

namespace ns
{
static_assert(1 == multiply_all<int>(std::array<std::int_least16_t, 5>{1, 1, 1, 1, 1}));
static_assert(100000 == multiply_all<int>(std::array<std::int_least16_t, 5>{10, 10, 10, 10, 10}));
static_assert(
        static_cast<__int128>(463838240001029376) * static_cast<__int128>(2134257315012840192)
        == multiply_all<__int128>(std::array<std::int_least16_t, 10>{
                1234, 5678, 9876, 5432, 1234, 5678, 9876, 5432, 1234, 5678}));
static_assert(
        static_cast<unsigned __int128>(463838240001029376) * static_cast<unsigned __int128>(2134257315012840192)
        == multiply_all<unsigned __int128>(std::array<std::uint_least16_t, 10>{
                1234, 5678, 9876, 5432, 1234, 5678, 9876, 5432, 1234, 5678}));
static_assert(
        static_cast<__float128>(463838240001029376) * static_cast<__float128>(2134257315012840192)
        == multiply_all<__float128>(std::array<float, 10>{1234, 5678, 9876, 5432, 1234, 5678, 9876, 5432, 1234, 5678}));

static_assert(5 == add_all<int>(std::array<std::int_least16_t, 5>{1, 1, 1, 1, 1}));
static_assert(50 == add_all<int>(std::array<std::int_least16_t, 5>{10, 10, 10, 10, 10}));
static_assert(
        51352
        == add_all<__int128>(std::array<std::int_least16_t, 10>{
                1234, 5678, 9876, 5432, 1234, 5678, 9876, 5432, 1234, 5678}));
static_assert(
        51352
        == add_all<unsigned __int128>(std::array<std::uint_least16_t, 10>{
                1234, 5678, 9876, 5432, 1234, 5678, 9876, 5432, 1234, 5678}));
static_assert(
        51352
        == add_all<__float128>(std::array<float, 10>{1234, 5678, 9876, 5432, 1234, 5678, 9876, 5432, 1234, 5678}));
}
