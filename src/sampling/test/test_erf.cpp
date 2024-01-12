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

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/sampling/erf.h>
#include <src/test/test.h>

#include <array>
#include <exception>
#include <iomanip>
#include <ios>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace ns::sampling::test
{
namespace
{
// For[i = 1/50, i < 1, i += 1/50,
//  Print[StringTemplate["{``L, ``L},"][i,
//    ScientificForm[N[InverseErf[i], 50],
//     NumberFormat -> (Row[{#1, "e", If[#3 == "", "0", #3]}] &)]]]]
template <typename T>
constexpr std::array INVERSE_ERF = std::to_array<std::array<T, 2>>({
        {0.02L, 1.7726395026678018482195112929313224840299869138666e-2L},
        {0.04L, 3.5463938968718641082209836734234907435800533402455e-2L},
        {0.06L, 5.3223829909765978023710895440807027106648031794332e-2L},
        {0.08L, 7.1017364833454740933678952589335595973692337768206e-2L},
        {0.10L, 8.8855990494257687015737250567791777572052244333197e-2L},
        {0.12L, 1.0675135602818441680418415862032366157158114337340e-1L},
        {0.14L, 1.2471536794266058347700965205066754588544249041608e-1L},
        {0.16L, 1.4276024817854752632491250241937863467781806314348e-1L},
        {0.18L, 1.6089859600789130339227268394276111706737629217858e-1L},
        {0.20L, 1.7914345462129167649274901662647187030390927701953e-1L},
        {0.22L, 1.9750838337227370288060001612909229800725174974097e-1L},
        {0.24L, 2.1600753678729464545449221534439871936296488192031e-1L},
        {0.26L, 2.3465575162492161076204889728668819204447671512447e-1L},
        {0.28L, 2.5346864348386572929532241400293445878523706708597e-1L},
        {0.30L, 2.7246271472675435562195759858756581266755846463101e-1L},
        {0.32L, 2.9165547581744204898195789816256667311916049007978e-1L},
        {0.34L, 3.1106558258078476766865874216382214004176371527047e-1L},
        {0.36L, 3.3071299240667360702855365239202747308739782520415e-1L},
        {0.38L, 3.5061914306308926506738192881390722874950940764075e-1L},
        {0.40L, 3.7080715859355792905824947752244913860430488316293e-1L},
        {0.42L, 3.9130208780283210702901160177202265983461830797914e-1L},
        {0.44L, 4.1213118214846543465371769260873918822421955427814e-1L},
        {0.46L, 4.3332422154706794135044570463490144251715229096232e-1L},
        {0.48L, 4.5491389879854004785364155852191412030024257150960e-1L},
        {0.50L, 4.7693627620446987338141835364313055980896974905947e-1L},
        {0.52L, 4.9943133175366345326164267598535422011131753247479e-1L},
        {0.54L, 5.2244361731717893285352929339417012045078590470794e-1L},
        {0.56L, 5.4602305813905509637966116314198870157785666155699e-1L},
        {0.58L, 5.7022593225950956936474072279649840144935403988631e-1L},
        {0.60L, 5.9511608144999485001930036016810825343961688627985e-1L},
        {0.62L, 6.2076642340926986269470763094283973396910466246265e-1L},
        {0.64L, 6.4726086087507356778569552752351260307199805216753e-1L},
        {0.66L, 6.7469672087225281689920346403004819361781262552046e-1L},
        {0.68L, 7.0318791282203616647863856959245649687698632873061e-1L},
        {0.70L, 7.3286907795921685221881746105801553557176747076776e-1L},
        {0.72L, 7.6390113173723643622162735649883701362040687681414e-1L},
        {0.74L, 7.9647880561170738536314283162310242412874769726486e-1L},
        {0.76L, 8.3084112847456012056939397503022302181762116635478e-1L},
        {0.78L, 8.6728635099387474053932140772705022745846114980739e-1L},
        {0.80L, 9.0619380243682322007116270309566286665086687474622e-1L},
        {0.82L, 9.4805697623234998774463106326994728220647435246924e-1L},
        {0.84L, 9.9353562834730426111305935025233398481497421350394e-1L},
        {0.86L,  1.0435418436397588726712998682285193875903823653292e0L},
        {0.88L,  1.0993909519492192652447347060305922130956112866969e0L},
        {0.90L,  1.1630871536766740867262542605629475934779325500021e0L},
        {0.92L,  1.2379219927112447060181604356171291766640827873706e0L},
        {0.94L,  1.3299219143360638040159346045180463526600932312927e0L},
        {0.96L,  1.4522197815622468501434208635071913776081156823245e0L},
        {0.98L,  1.6449763571331870501772034352495116246653430362888e0L}
});

template <typename T>
void compare_erf_inv(const T arg, const T erf, const T erf_inverse, const T precision)
{
        if (arg == erf_inverse)
        {
                return;
        }

        const T abs = std::abs(arg - erf_inverse);

        if (arg == 0 || erf_inverse == 0)
        {
                if (!(abs < precision))
                {
                        error("Absolute erf_inv error " + to_string(abs) + " is greater than " + to_string(precision));
                }
                return;
        }

        const T p = [&]
        {
                if (erf > 1 - 100 * Limits<T>::epsilon() || erf < -1 + 100 * Limits<T>::epsilon())
                {
                        return precision * 10;
                }
                return precision;
        }();

        const T rel = abs / std::max(std::abs(arg), std::abs(erf_inverse));
        if (!(rel < p))
        {
                error("Relative erf_inv error " + to_string(rel) + " is greater than " + to_string(p));
        }
}

template <typename T>
void test_erf_inv(const T arg, const T erf, const T erf_inverse, const T precision)
{
        if (erf == 1)
        {
                if (!(erf_inverse == Limits<T>::infinity()))
                {
                        error("erf inverse is not inf for erf 1");
                }
                return;
        }

        if (erf == -1)
        {
                if (!(erf_inverse == -Limits<T>::infinity()))
                {
                        error("erf inverse is not inf for erf 1");
                }
                return;
        }

        if (erf == 0)
        {
                if (!(erf_inverse == 0))
                {
                        error("erf inverse is not 0 for erf 0");
                }
                return;
        }

        compare_erf_inv(arg, erf, erf_inverse, precision);
}

template <typename T>
void test_erf_inv(const std::type_identity_t<T> arg, const std::type_identity_t<T> precision)
{
        static_assert(std::is_floating_point_v<T>);

        const T erf = std::erf(arg);
        const T erf_inverse = erf_inv(erf);

        try
        {
                test_erf_inv(arg, erf, erf_inverse, precision);
        }
        catch (const std::exception& e)
        {
                std::ostringstream oss;
                oss << e.what() << '\n';
                oss << std::scientific;
                oss << std::setprecision(Limits<T>::max_digits10());
                oss << "arg = " << arg << '\n';
                oss << "erf_inv = " << erf_inverse << '\n';
                oss << "erf = " << erf;
                error(oss.str());
        }
}

template <typename T>
void test_erf_inv(const std::type_identity_t<T> precision, const int divisions)
{
        static_assert(std::is_floating_point_v<T>);

        for (int i = -10; i < 10; ++i)
        {
                for (int j = 0; j < divisions; ++j)
                {
                        test_erf_inv<T>(i + static_cast<T>(j) / divisions, precision);
                }
        }
}

template <typename T>
void test_erf_inv_array(const std::type_identity_t<T> precision)
{
        static_assert(std::is_floating_point_v<T>);

        if (!std::isnan(erf_inv<T>(-2)))
        {
                error("erf_inv(-2) is not NAN");
        }

        if (!std::isnan(erf_inv<T>(2)))
        {
                error("erf_inv(2) is not NAN");
        }

        if (!(erf_inv<T>(-1) == -Limits<T>::infinity()))
        {
                error("erf_inv(-1) is not -infinity");
        }

        if (!(erf_inv<T>(0) == 0))
        {
                error("erf_inv(0) is not 0");
        }

        if (!(erf_inv<T>(1) == Limits<T>::infinity()))
        {
                error("erf_inv(1) is not infinity");
        }

        for (const std::array<T, 2>& v : INVERSE_ERF<T>)
        {
                const T e1 = std::abs(erf_inv(v[0]) - v[1]) / std::max(std::abs(erf_inv(v[0])), std::abs(v[1]));
                const T e2 = std::abs(erf_inv(-v[0]) - (-v[1])) / std::max(std::abs(erf_inv(-v[0])), std::abs(v[1]));
                if (!(e1 < precision && e2 < precision))
                {
                        error("Relative erf_inv error e1 = " + to_string(e1) + " e2 = " + to_string(e2)
                              + " are greater than " + to_string(precision) + ", a = " + to_string(v[0])
                              + ", f = " + to_string(v[1]) + ", erf_inv = " + to_string(erf_inv(v[0])));
                }
        }
}

template <typename T>
void test_performance()
{
        constexpr int DATA_SIZE = 10'000;
        constexpr int COUNT = 1000;

        const std::vector<T> data = [&]
        {
                PCG engine;
                std::uniform_real_distribution<T> urd(-1.0001, 1.0001);
                std::vector<T> res;
                res.reserve(DATA_SIZE);
                for (int i = 0; i < DATA_SIZE; ++i)
                {
                        res.push_back(urd(engine));
                }
                return res;
        }();

        const Clock::time_point time_point = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                for (const T& v : data)
                {
                        do_not_optimize(erf_inv(v));
                }
        }
        const double duration = duration_from(time_point);

        const long long p = std::llround(COUNT * data.size() / duration);
        LOG(std::string("Inverse error function <") + type_name<T>() + ">: " + to_string_digit_groups(p) + " o/s");
}

void test_erf()
{
        LOG("Test erf_inv array");
        test_erf_inv_array<float>(0.003);
        test_erf_inv_array<double>(0.003);
        test_erf_inv_array<long double>(0.003);

        LOG("Test erf_inv function");
        test_erf_inv<float>(0.005, 200);
        test_erf_inv<double>(0.005, 200);
        test_erf_inv<long double>(0.005, 200);

        LOG("Test erf_inv passed");
}

void test_erf_performance()
{
        test_performance<float>();
        test_performance<double>();
        test_performance<long double>();
}

TEST_SMALL("Inverse Error Function", test_erf)
TEST_PERFORMANCE("Inverse Error Function", test_erf_performance)
}
}
