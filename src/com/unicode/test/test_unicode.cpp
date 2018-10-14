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

#include "test_unicode.h"

#include "com/error.h"
#include "com/print.h"
#include "com/unicode/unicode.h"

void test_unicode()
{
        std::string utf8;

        for (char32_t c1 = 0; c1 <= 0x10FFFF; ++c1)
        {
                utf8 = unicode::utf32_to_utf8(c1);

                char32_t c2 = unicode::utf8_to_utf32(utf8);

                if (c2 != c1)
                {
                        std::string e;
                        e += "Error Unicode converting.\n";
                        e += "UTF-32: " + to_string(static_cast<long long>(c1)) + "\n";
                        e += "UTF-8: " + utf8;
                        e += "UTF-32: " + to_string(static_cast<long long>(c2)) + "\n";
                        error(e);
                }
        }

        //

        if ("\xEF\xBF\xBD" != unicode::utf32_to_utf8(char32_t(0xFFFFFF)))
        {
                error("Error UTF-8 replacement character");
        }

        size_t i;

        i = 0;
        if (0xFFFD != unicode::read_utf8_as_utf32("\x96\x96", i))
        {
                error("Error UTF-32 replacement character");
        }

        i = 0;
        if (0xFFFD != unicode::read_utf8_as_utf32("\xE2\x88", i))
        {
                error("Error UTF-32 replacement character");
        }

        //

        if (0x222B != unicode::utf8_to_utf32("\xE2\x88\xAB"))
        {
                error("Error UTF-8 to UTF-32");
        }

        if ("\xE2\x88\xAB" != unicode::utf32_to_utf8(char32_t(0x222B)))
        {
                error("Error UTF-32 to UTF-8");
        }
}
