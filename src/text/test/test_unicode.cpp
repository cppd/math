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

#include "test_unicode.h"

#include "../unicode.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>

namespace ns::text
{
namespace
{
void test_utf32_to_utf8_to_utf32()
{
        LOG("UTF-32 to UTF-8 to UTF-32");

        std::string utf8;

        for (char32_t c1 = 0; c1 <= 0x10FFFF; ++c1)
        {
                utf8 = unicode::utf32_to_utf8(c1);

                char32_t c2 = unicode::utf8_to_utf32(utf8);

                if (c2 != c1)
                {
                        std::string e;
                        e += "Error Unicode converting.\n";
                        e += "UTF-32: " + unicode::utf32_to_number_string(c1) + "\n";
                        e += "UTF-8: " + unicode::utf8_to_number_string(utf8) + "\n";
                        e += "UTF-32: " + unicode::utf32_to_number_string(c2) + "\n";
                        error(e);
                }
        }
}

void test_utf32_replacement_character()
{
        LOG("UTF-32 replacement character");

        if (reinterpret_cast<const char*>(u8"\U0000FFFD") != unicode::utf32_to_utf8(char32_t(0xFFFFFF)))
        {
                error("Error UTF-8 replacement character");
        }
}

void test_utf8_replacement_character_and_self_synchronizing()
{
        LOG("UTF-8 replacement character and self-synchronizing");

        {
                std::string s = reinterpret_cast<const char*>(u8"\U0000222B\U00002211");
                s.erase(0, 1);

                ASSERT(s.size() == 5);

                size_t i = 0;

                if (0xFFFD != unicode::utf8_to_utf32(s, &i))
                {
                        error("Error UTF-32 replacement character");
                }
                if (i != 1)
                {
                        error("Error UTF-8 string index " + to_string(i));
                }

                if (0xFFFD != unicode::utf8_to_utf32(s, &i))
                {
                        error("Error UTF-32 replacement character");
                }
                if (i != 2)
                {
                        error("Error UTF-8 string index " + to_string(i));
                }

                if (0x2211 != unicode::utf8_to_utf32(s, &i))
                {
                        error("Error reading UTF-32");
                }
                if (i != s.size())
                {
                        error("Error UTF-8 string index " + to_string(i));
                }
        }

        {
                std::string s = reinterpret_cast<const char*>(u8"\U0000222B\U00002211");
                s.erase(2, 1);

                ASSERT(s.size() == 5);

                size_t i = 0;

                if (0xFFFD != unicode::utf8_to_utf32(s, &i))
                {
                        error("Error UTF-32 replacement character");
                }
                if (i != 1)
                {
                        error("Error UTF-8 string index " + to_string(i));
                }

                if (0xFFFD != unicode::utf8_to_utf32(s, &i))
                {
                        error("Error UTF-32 replacement character");
                }
                if (i != 2)
                {
                        error("Error UTF-8 string index " + to_string(i));
                }

                if (0x2211 != unicode::utf8_to_utf32(s, &i))
                {
                        error("Error reading UTF-32");
                }
                if (i != s.size())
                {
                        error("Error UTF-8 string index " + to_string(i));
                }
        }
}

void test_utf32_to_utf8()
{
        LOG("UTF-32 to UTF-8");

        if (reinterpret_cast<const char*>(u8"\U0000222B") != unicode::utf32_to_utf8(char32_t(0x222B)))
        {
                error("Error UTF-32 to UTF-8");
        }
}

void test_utf8_to_utf32()
{
        LOG("UTF-8 to UTF-32");

        if (0x222B != unicode::utf8_to_utf32(reinterpret_cast<const char*>(u8"\U0000222B")))
        {
                error("Error UTF-8 to UTF-32");
        }
}
}

void test_unicode()
{
        test_utf32_to_utf8_to_utf32();
        test_utf32_replacement_character();
        test_utf8_replacement_character_and_self_synchronizing();
        test_utf32_to_utf8();
        test_utf8_to_utf32();
}
}
