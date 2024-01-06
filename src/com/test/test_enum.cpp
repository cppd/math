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

#include <src/com/enum.h>

#include <type_traits>

namespace ns
{
namespace
{
template <typename Underlying, Underlying VALUE, typename Result>
struct Test final
{
        enum class Enum : Underlying
        {
                E = VALUE
        };
        static_assert(std::is_same_v<Result, decltype(enum_to_int(Enum::E))>);
        static_assert(enum_to_int(Enum::E) == VALUE);
};

template struct Test<signed char, 100, int>;
template struct Test<signed char, -100, int>;
template struct Test<unsigned char, 100, unsigned>;
template struct Test<char, 100, std::conditional_t<std::is_signed_v<char>, int, unsigned>>;
template struct Test<int, 1'000'000'000, int>;
template struct Test<int, -1'000'000'000, int>;
template struct Test<unsigned, 1'000'000'000, unsigned>;
template struct Test<long long, 1'000'000'000'000, long long>;
template struct Test<long long, -1'000'000'000'000, long long>;
template struct Test<unsigned long long, 1'000'000'000'000, unsigned long long>;
template struct Test<__int128, __int128{1'000'000'000'000} * __int128{1'000'000'000'000}, __int128>;
template struct Test<__int128, __int128{-1'000'000'000'000} * __int128{1'000'000'000'000}, __int128>;
template struct Test<unsigned __int128, __int128{1'000'000'000'000} * __int128{1'000'000'000'000}, unsigned __int128>;
}
}
