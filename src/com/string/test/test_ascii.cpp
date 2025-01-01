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

#include <src/com/string/ascii.h>

namespace ns
{
static_assert(ascii::to_upper('a') == 'A');
static_assert(ascii::to_upper('z') == 'Z');
static_assert(ascii::to_lower('A') == 'a');
static_assert(ascii::to_lower('Z') == 'z');
static_assert(ascii::is_space(' '));
static_assert(ascii::is_space('\t'));
static_assert(ascii::is_space('\n'));
static_assert(ascii::is_space('\v'));
static_assert(ascii::is_space('\f'));
static_assert(ascii::is_space('\r'));
static_assert(!ascii::is_space('.'));
static_assert(!ascii::is_space('\0'));
static_assert(ascii::is_not_space(','));
static_assert(!ascii::is_not_space('\n'));
static_assert(ascii::is_blank(' '));
static_assert(ascii::is_blank('\t'));
static_assert(!ascii::is_blank('\r'));
static_assert(!ascii::is_blank('\0'));
static_assert(ascii::is_not_blank(','));
static_assert(!ascii::is_not_blank('\t'));
static_assert(ascii::is_digit('0'));
static_assert(ascii::is_digit('9'));
static_assert(!ascii::is_digit('a'));
static_assert(!ascii::is_digit('Z'));
static_assert(!ascii::is_digit(','));
static_assert(!ascii::is_digit('\"'));
static_assert(ascii::char_to_int('0') == 0);
static_assert(ascii::char_to_int('1') == 1);
static_assert(ascii::char_to_int('2') == 2);
static_assert(ascii::char_to_int('3') == 3);
static_assert(ascii::char_to_int('4') == 4);
static_assert(ascii::char_to_int('5') == 5);
static_assert(ascii::char_to_int('6') == 6);
static_assert(ascii::char_to_int('7') == 7);
static_assert(ascii::char_to_int('8') == 8);
static_assert(ascii::char_to_int('9') == 9);
}
