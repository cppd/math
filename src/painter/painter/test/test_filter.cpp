/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "../filter.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/test/test.h>

namespace ns::painter
{
namespace
{
template <typename T>
void compare(T a, T b)
{
        if (a == b)
        {
                return;
        }
        T abs = std::abs(a - b);
        T max = std::max(std::abs(a), std::abs(b));
        if (!(abs / max < T(1e-5)))
        {
                error("Values are not equal: " + to_string(a) + " and " + to_string(b));
        }
}

template <typename T, std::size_t N>
void compare(T alpha, T radius, const Vector<N, T>& p, T value)
{
        compare(GaussianFilter<T>(alpha, radius).compute(p), value);
}

template <typename T>
void test_filter()
{
        //filter[alpha_,radius_,list_]:=Module[{e,m,k},
        //e=Exp[-alpha*radius*radius];
        //m=1;
        //Do[m*=Max[0,Exp[-alpha*v*v]-e],{v, list}];
        //m];
        //N[filter[1/2,5,{-1,1,2}],50]
        //N[filter[1/2,5,{-1/10,1/10,2/10}],50]
        //N[filter[1,5,{-1,1,2}],50]
        //N[filter[1,5,{-1,1,10}],50]
        //N[filter[2,5,{-1,1,2}],50]

        compare<T>(0.5, 5, Vector<3, T>(-1, 1, 2), 0.049785085622862959813327490179279500949316447044202L);
        compare<T>(0.5, 5, Vector<3, T>(-0.1, 0.1, 0.2), 0.97043457473385012663134014779783236545118566333793L);
        compare<T>(1, 5, Vector<3, T>(-1, 1, 2), 0.0024787521745996771472747227557752970814643055320281L);
        compare<T>(1, 5, Vector<3, T>(-1, 1, 10), 0L);
        compare<T>(2, 5, Vector<3, T>(-1, 1, 2), 6.1442123533282097551321665850793322408217006692917e-6L);
}

void test()
{
        test_filter<float>();
        test_filter<double>();
}

TEST_SMALL("Painter filter", test)
}
}
