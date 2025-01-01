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

#include <src/color/color.h>
#include <src/shading/ggx/fresnel.h>

#include <cstddef>

namespace ns::shading::ggx
{
namespace
{
// f0=1/2;
// n=3;
// N[(n-1)*Integrate[Power[Sin[x],n-2]*Cos[x]*(f0+(1-f0)*Power[1-Cos[x],5]),{x,0,Pi/2}],50]
template <typename Color>
struct Test final
{
        template <std::size_t N>
        static constexpr bool equal(const Color& a, const Color& b)
        {
                const Color v = fresnel_cosine_weighted_average<N>(a);
                return v.equal_to_relative(b, 1e-6) && v.equal_to_absolute(b, 1e-7);
        }

        static_assert(equal<3>(Color(0.5), Color(0.52380952380952380952380952380952380952380952380952L)));
        static_assert(equal<4>(Color(0.5), Color(0.53414661141500179372961514423452078857611831226709L)));
        static_assert(equal<5>(Color(0.5), Color(0.54365079365079365079365079365079365079365079365079L)));
        static_assert(equal<6>(Color(0.5), Color(0.55243921935527907324916649305826501397164100330401L)));
        static_assert(equal<7>(Color(0.5), Color(0.56060606060606060606060606060606060606060606060606L)));
        static_assert(equal<8>(Color(0.5), Color(0.56822816691139623250880668008422595808350231323425L)));
        static_assert(equal<9>(Color(0.5), Color(0.57536907536907536907536907536907536907536907536908L)));
};

template struct Test<color::Color>;
template struct Test<color::Spectrum>;
}
}
