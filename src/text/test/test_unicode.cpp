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

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/test/test.h>
#include <src/text/unicode.h>

#include <cstddef>
#include <string>
#include <vector>

namespace ns::text::unicode
{
namespace
{
void test_utf32_to_utf8_to_utf32()
{
        LOG("UTF-32 to UTF-8 to UTF-32");

        for (char32_t c1 = 0; c1 <= 0x10FFFF; ++c1)
        {
                const std::string utf8 = utf32_to_utf8(c1);
                const char32_t c2 = utf8_to_utf32(utf8);

                if (c2 != c1)
                {
                        std::string e;
                        e += "Error Unicode converting.\n";
                        e += "UTF-32: " + utf32_to_number_string(c1) + "\n";
                        e += "UTF-8: " + utf8_to_number_string(utf8) + "\n";
                        e += "UTF-32: " + utf32_to_number_string(c2) + "\n";
                        error(e);
                }
        }
}

void test_utf32_replacement_character()
{
        LOG("UTF-32 replacement character");

        if (reinterpret_cast<const char*>(u8"\U0000FFFD") != utf32_to_utf8(char32_t{0xFFFFFF}))
        {
                error("Error UTF-8 replacement character");
        }
}

void test_utf8_replacement_character_and_self_synchronizing(const std::string& s)
{
        ASSERT(s.size() == 5);

        std::size_t i = 0;

        if (REPLACEMENT_CHARACTER != utf8_to_utf32(s, &i))
        {
                error("Error UTF-32 replacement character");
        }
        if (i != 1)
        {
                error("Error UTF-8 string index " + to_string(i));
        }

        if (REPLACEMENT_CHARACTER != utf8_to_utf32(s, &i))
        {
                error("Error UTF-32 replacement character");
        }
        if (i != 2)
        {
                error("Error UTF-8 string index " + to_string(i));
        }

        if (0x2211 != utf8_to_utf32(s, &i))
        {
                error("Error reading UTF-32");
        }
        if (i != s.size())
        {
                error("Error UTF-8 string index " + to_string(i));
        }
}

void test_utf8_replacement_character_and_self_synchronizing()
{
        LOG("UTF-8 replacement character and self-synchronizing");

        const auto* const string = reinterpret_cast<const char*>(u8"\U0000222B\U00002211");

        std::vector<std::string> strings;

        strings.emplace_back(string);
        strings.back().erase(0, 1);

        strings.emplace_back(string);
        strings.back().erase(2, 1);

        for (const auto& s : strings)
        {
                test_utf8_replacement_character_and_self_synchronizing(s);
        }
}

void test_utf32_to_utf8()
{
        LOG("UTF-32 to UTF-8");

        if (reinterpret_cast<const char*>(u8"\U0000222B") != utf32_to_utf8(char32_t{0x222B}))
        {
                error("Error UTF-32 to UTF-8");
        }
}

void test_utf8_to_utf32()
{
        LOG("UTF-8 to UTF-32");

        if (0x222B != utf8_to_utf32(reinterpret_cast<const char*>(u8"\U0000222B")))
        {
                error("Error UTF-8 to UTF-32");
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

TEST_SMALL("Unicode", test_unicode)
}
}
