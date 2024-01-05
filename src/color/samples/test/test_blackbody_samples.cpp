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

#include <src/color/samples/blackbody_samples.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <exception>
#include <string>
#include <utility>
#include <vector>

namespace ns::color
{
namespace
{
class Exception final : public std::exception
{
        std::string text_;

public:
        explicit Exception(std::string&& text)
                : text_(std::move(text))
        {
        }

        [[nodiscard]] const char* what() const noexcept override
        {
                return text_.c_str();
        }
};

void compare_a(const double t, const int min, const int max, const unsigned count)
{
        if (!(max > min))
        {
                error("Error min " + to_string(min) + " and max " + to_string(max));
        }

        if (!(count > 0))
        {
                error("Error count " + to_string(count));
        }

        const std::vector<double> a = blackbody_a_samples(min, max, count);
        const std::vector<double> blackbody = blackbody_samples(t, min, max, count);

        if (a.size() != count || blackbody.size() != count)
        {
                error("Error sample count A " + to_string(a.size()) + " and blackbody " + to_string(blackbody.size()));
        }

        for (std::size_t i = 0; i < a.size(); ++i)
        {
                if (!(a[i] >= 0))
                {
                        error("A " + to_string(a[i]) + " is not positive and not zero");
                }

                if (!(blackbody[i] >= 0))
                {
                        error("Blackbody " + to_string(blackbody[i]) + " is not positive and not zero");
                }

                if (a[i] == blackbody[i])
                {
                        continue;
                }

                const double abs = std::abs(a[i] - blackbody[i]);
                const double rel = abs / std::max(std::abs(a[i]), std::abs(blackbody[i]));
                if (!(rel < 2.5e-5))
                {
                        throw Exception(
                                "A " + to_string(a[i]) + " and blackbody " + to_string(blackbody[i])
                                + " are not equal, relative error " + to_string(rel));
                }
        }
}

void check_equal_to_a(const double t, const int min, const int max, const unsigned count)
{
        try
        {
                compare_a(t, min, max, count);
        }
        catch (const Exception& e)
        {
                error(e.what());
        }
}

void check_not_equal_to_a(const double t, const int min, const int max, const unsigned count)
{
        try
        {
                compare_a(t, min, max, count);
        }
        catch (const Exception&)
        {
                return;
        }
        error("Samples T " + to_string(t) + " are equal to A");
}

void compare(const double a, const double b)
{
        if (a == b)
        {
                return;
        }

        const double rel = std::abs(a - b) / std::max(std::abs(a), std::abs(b));
        if (rel <= 1e-4)
        {
                return;
        }

        error(to_string(a) + " and " + to_string(b) + " are not equal, relative error " + to_string(rel));
}

void test_blackbody_a()
{
        constexpr int MIN = 300;
        constexpr int MAX = 1000;
        constexpr int COUNT = 100;

        check_equal_to_a(2855.5, MIN, MAX, COUNT);

        check_not_equal_to_a(2500, MIN, MAX, COUNT);
        check_not_equal_to_a(2850, MIN, MAX, COUNT);
        check_not_equal_to_a(2860, MIN, MAX, COUNT);
        check_not_equal_to_a(3000, MIN, MAX, COUNT);
}

void test_blackbody()
{
        // h = 6.62607015`30*(10^-34);
        // kb = 1.380649`30*(10^-23);
        // c = 299792458;
        // sample[from_, to_, t_] :=
        //   N[Integrate[(2*h*c*c)/((l^5)*(Exp[(h*c)/(l*kb*t)] - 1)), {l,
        //       from*(10^-9), to*(10^-9)}], 20]/((to - from)*10^-9);
        // samples[t_] :=
        //   For[i = 300, i <= 900, i += 100,
        //    Print[StringTemplate["compare(s[``], ``);"][(i - 300)/100,
        //      sample[i, i + 100, t]]]];
        // samples[2500]
        // Print[]
        // samples[5000]
        // Print[]
        // samples[10000]

        constexpr int FROM = 300;
        constexpr int TO = 1000;
        constexpr int COUNT = 7;

        const auto create_samples = [&](const double t)
        {
                std::vector<double> samples = blackbody_samples(t, FROM, TO, COUNT);
                if (samples.size() != static_cast<std::size_t>(COUNT))
                {
                        error("Sample count " + to_string(samples.size()) + " is not equal to " + to_string(COUNT));
                }
                return samples;
        };

        {
                const std::vector<double> s = create_samples(2500);

                compare(s[0], 2219657013.027439273);
                compare(s[1], 19483839638.005915767);
                compare(s[2], 68854464152.855891984);
                compare(s[3], 146928790023.21432077);
                compare(s[4], 232952898577.32847785);
                compare(s[5], 307265555756.99660445);
                compare(s[6], 359901370617.50418974);
        }
        {
                const std::vector<double> s = create_samples(5000);

                compare(s[0], 6078107017608.684778);
                compare(s[1], 10674670821992.012707);
                compare(s[2], 12620210738678.418189);
                compare(s[3], 12372781941105.233611);
                compare(s[4], 11050257294927.665769);
                compare(s[5], 9412601727248.6755504);
                compare(s[6], 7831446403960.3563022);
        }
        {
                const std::vector<double> s = create_samples(10000);

                compare(s[0], 374768627776526.39008);
                compare(s[1], 275904770099344.50964);
                compare(s[2], 187828065206941.95844);
                compare(s[3], 126860939212006.91784);
                compare(s[4], 86954109703749.831053);
                compare(s[5], 60896183490731.818712);
                compare(s[6], 43620954692038.648322);
        }
}

void test()
{
        LOG("Test blackbody samples");

        test_blackbody();
        test_blackbody_a();

        LOG("Test blackbody samples passed");
}

TEST_SMALL("Blackbody Samples", test)
}
}
