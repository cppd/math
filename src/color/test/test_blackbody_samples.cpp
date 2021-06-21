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

#include "../blackbody_samples.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/test/test.h>

namespace ns::color
{
namespace
{
void check(double a, double b)
{
        if (a == b)
        {
                return;
        }

        const double rel = std::abs(a - b) / std::max(std::abs(a), std::abs(b));
        if (!(rel <= 1e-4))
        {
                error(to_string(a) + " and " + to_string(b) + " are not equal, relative error " + to_string(rel));
        }
}

void test()
{
        //h = 6.62607015`30*(10^-34);
        //kb = 1.380649`30*(10^-23);
        //c = 299792458;
        //sample[from_, to_, t_] :=
        //  N[Integrate[(2*h*c*c)/((l^5)*(Exp[(h*c)/(l*kb*t)] - 1)), {l,
        //      from*(10^-9), to*(10^-9)}], 20]/((to - from)*10^-9);
        //samples[t_] :=
        //  For[i = 300, i <= 900, i += 100,
        //   Print[StringTemplate["check(s[``], ``);"][(i - 300)/100,
        //     sample[i, i + 100, t]]]];
        //samples[2500]
        //Print[]
        //samples[5000]
        //Print[]
        //samples[10000]

        LOG("Test blackbody samples");

        constexpr int from = 300;
        constexpr int to = 1000;
        constexpr int count = 7;

        const auto create_samples = [&](double t)
        {
                std::vector<double> samples = blackbody_samples(t, from, to, count);
                if (samples.size() != static_cast<std::size_t>(count))
                {
                        error("Sample count " + to_string(samples.size()) + " is not equal to " + to_string(count));
                }
                return samples;
        };

        {
                const std::vector<double> s = create_samples(2500);

                check(s[0], 2219657013.027439273);
                check(s[1], 19483839638.005915767);
                check(s[2], 68854464152.855891984);
                check(s[3], 146928790023.21432077);
                check(s[4], 232952898577.32847785);
                check(s[5], 307265555756.99660445);
                check(s[6], 359901370617.50418974);
        }
        {
                const std::vector<double> s = create_samples(5000);

                check(s[0], 6078107017608.684778);
                check(s[1], 10674670821992.012707);
                check(s[2], 12620210738678.418189);
                check(s[3], 12372781941105.233611);
                check(s[4], 11050257294927.665769);
                check(s[5], 9412601727248.6755504);
                check(s[6], 7831446403960.3563022);
        }
        {
                const std::vector<double> s = create_samples(10000);

                check(s[0], 374768627776526.39008);
                check(s[1], 275904770099344.50964);
                check(s[2], 187828065206941.95844);
                check(s[3], 126860939212006.91784);
                check(s[4], 86954109703749.831053);
                check(s[5], 60896183490731.818712);
                check(s[6], 43620954692038.648322);
        }

        LOG("Test blackbody samples passed");
}

TEST_SMALL("Blackbody Samples", test)
}
}
