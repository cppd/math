/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "../print.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/type/limit.h>
#include <src/test/test.h>

#include <string>
#include <string_view>

namespace ns
{
namespace
{
template <typename T>
void compare(const T value, const std::string_view str)
{
        if (!(to_string(value) == str))
        {
                error("Error converting number to string \"" + std::string(str) + "\"");
        }
}

template <typename T>
void compare_g(const T value, const std::string_view str)
{
        if (!(to_string_digit_groups(value) == str))
        {
                error("Error converting number to string \"" + std::string(str) + "\"");
        }
}

template <typename T>
void compare_g(const T value, const char separator, const std::string_view str)
{
        if (!(to_string_digit_groups(value, separator) == str))
        {
                error("Error converting number to string \"" + std::string(str) + "\"");
        }
}

template <typename T>
void compare_binary(const T value, const std::string& str)
{
        if (!(to_string_binary(value) == str))
        {
                error("Error converting number to string \"" + std::string(str) + "\"");
        }

        const std::string prefix = "0b";
        const std::string str_prefix = prefix + str;
        if (!(to_string_binary(value, prefix) == str_prefix))
        {
                error("Error converting number to string \"" + std::string(str_prefix) + "\"");
        }
}

void test_int()
{
        {
                constexpr int VALUE{0};
                compare(VALUE, "0");
                compare_g(VALUE, "0");
                compare_g(VALUE, '\'', "0");
        }
        {
                constexpr unsigned VALUE{0};
                compare(VALUE, "0");
                compare_g(VALUE, "0");
                compare_g(VALUE, '\'', "0");
        }

        {
                constexpr int VALUE{1};
                compare(VALUE, "1");
                compare_g(VALUE, "1");
                compare_g(VALUE, '\'', "1");
        }
        {
                constexpr int VALUE{-1};
                compare(VALUE, "-1");
                compare_g(VALUE, "-1");
                compare_g(VALUE, '\'', "-1");
        }
        {
                constexpr unsigned VALUE{1};
                compare(VALUE, "1");
                compare_g(VALUE, "1");
                compare_g(VALUE, '\'', "1");
        }

        {
                constexpr int VALUE{12345};
                compare(VALUE, "12345");
                compare_g(VALUE, "12 345");
                compare_g(VALUE, '\'', "12'345");
        }
        {
                constexpr int VALUE{-12345};
                compare(VALUE, "-12345");
                compare_g(VALUE, "-12 345");
                compare_g(VALUE, '\'', "-12'345");
        }
        {
                constexpr unsigned VALUE{12345};
                compare(VALUE, "12345");
                compare_g(VALUE, "12 345");
                compare_g(VALUE, '\'', "12'345");
        }

        {
                constexpr int VALUE{2'147'483'647};
                compare(VALUE, "2147483647");
                compare_g(VALUE, "2 147 483 647");
        }
        {
                constexpr int VALUE{-2'147'483'648};
                compare(VALUE, "-2147483648");
                compare_g(VALUE, "-2 147 483 648");
        }
        {
                constexpr unsigned VALUE{4'294'967'295};
                compare(VALUE, "4294967295");
                compare_g(VALUE, "4 294 967 295");
        }
}

void test_long_long()
{
        {
                constexpr long long VALUE{1'234'567'890'987'654'321};
                compare(VALUE, "1234567890987654321");
                compare_g(VALUE, "1 234 567 890 987 654 321");
        }
        {
                constexpr long long VALUE{-1'234'567'890'987'654'321};
                compare(VALUE, "-1234567890987654321");
                compare_g(VALUE, "-1 234 567 890 987 654 321");
        }
        {
                constexpr unsigned long long VALUE{1'234'567'890'987'654'321};
                compare(VALUE, "1234567890987654321");
                compare_g(VALUE, "1 234 567 890 987 654 321");
        }
}

void test_int128()
{
        {
                constexpr __int128 VALUE{square(static_cast<__int128>(1'234'567'890'987'654'321))};
                compare(VALUE, "1524157877457704723228166437789971041");
                compare_g(VALUE, "1 524 157 877 457 704 723 228 166 437 789 971 041");
        }
        {
                constexpr __int128 VALUE{-square(static_cast<__int128>(1'234'567'890'987'654'321))};
                compare(VALUE, "-1524157877457704723228166437789971041");
                compare_g(VALUE, "-1 524 157 877 457 704 723 228 166 437 789 971 041");
        }
        {
                constexpr unsigned __int128 VALUE{square(static_cast<unsigned __int128>(1'234'567'890'987'654'321))};
                compare(VALUE, "1524157877457704723228166437789971041");
                compare_g(VALUE, "1 524 157 877 457 704 723 228 166 437 789 971 041");
        }

        {
                constexpr __int128 VALUE{Limits<__int128>::max()};
                compare(VALUE, "170141183460469231731687303715884105727");
                compare_g(VALUE, "170 141 183 460 469 231 731 687 303 715 884 105 727");
        }
        {
                constexpr __int128 VALUE{Limits<__int128>::lowest()};
                compare(VALUE, "-170141183460469231731687303715884105728");
                compare_g(VALUE, "-170 141 183 460 469 231 731 687 303 715 884 105 728");
        }
        {
                constexpr unsigned __int128 VALUE{Limits<unsigned __int128>::max()};
                compare(VALUE, "340282366920938463463374607431768211455");
                compare_g(VALUE, "340 282 366 920 938 463 463 374 607 431 768 211 455");
        }
}

void test_binary()
{
        compare_binary(0u, "0");

        compare_binary(1u, "1");

        compare_binary(1'234'567'890'987'654'321ull, "1000100100010000100001111010010110001011011000001110010110001");

        compare_binary(0xffff'ffff'ffff'ffffull, "1111111111111111111111111111111111111111111111111111111111111111");
}

void test()
{
        test_int();
        test_long_long();
        test_int128();
        test_binary();
}

TEST_SMALL("Print", test)
}
}
