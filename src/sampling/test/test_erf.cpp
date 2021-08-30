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

#include "../erf.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/time.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/test/test.h>

#include <random>
#include <sstream>

namespace ns::sampling::test
{
namespace
{
template <typename T>
std::string make_string(T arg, T inverse, T erf)
{
        static_assert(std::is_floating_point_v<T>);

        std::ostringstream oss;
        oss << std::scientific;
        oss << std::setprecision(limits<T>::max_digits10);
        oss << "arg = " << arg << ", erf_inv = " << inverse << ", erf = " << erf;
        return oss.str();
}

template <typename T>
void test_erf_inv(std::type_identity_t<T> arg, std::type_identity_t<T> precision)
{
        static_assert(std::is_floating_point_v<T>);

        const T erf = std::erf(arg);
        const T inverse = erf_inv(erf);

        try
        {
                if (erf == 1)
                {
                        if (!(inverse == limits<T>::infinity()))
                        {
                                error("erf inverse is not inf for erf 1");
                        }
                }
                else if (erf == -1)
                {
                        if (!(inverse == -limits<T>::infinity()))
                        {
                                error("erf inverse is not inf for erf 1");
                        }
                }
                else if (erf == 0)
                {
                        if (!(inverse == 0))
                        {
                                error("erf inverse is not 0 for erf 0");
                        }
                }
                else if (arg == inverse)
                {
                        return;
                }
                else if (arg == 0 || inverse == 0)
                {
                        if (!(std::abs(arg - inverse) < precision))
                        {
                                error("Absolute erf_inv error is greater than " + to_string(precision));
                        }
                }
                else
                {
                        T p = precision;

                        if (erf > 1 - 100 * limits<T>::epsilon() || erf < -1 + 100 * limits<T>::epsilon())
                        {
                                p *= 10;
                        }

                        T e = std::abs((arg - inverse) / arg);
                        if (!(e < p))
                        {
                                error("Relative erf_inv error " + to_string(e) + " is greater than " + to_string(p));
                        }
                }
        }
        catch (const std::exception& e)
        {
                error(std::string(e.what()) + "\n" + make_string(arg, inverse, erf));
        }
}

template <typename T>
void test_erf_inv(std::type_identity_t<T> precision, int divisions)
{
        static_assert(std::is_floating_point_v<T>);

        for (int i = -10; i < 10; ++i)
        {
                for (int j = 0; j < divisions; ++j)
                {
                        test_erf_inv<T>(T(i) + T(j) / T(divisions), precision);
                }
        }
}

template <typename T>
void test_erf_inv_from_array(std::type_identity_t<T> precision)
{
        static_assert(std::is_floating_point_v<T>);

        static constexpr std::array<T, 2> DATA[] = {
                {0.02, 0.017726395026678018482195112929313224840299869138666L},
                {0.04, 0.035463938968718641082209836734234907435800533402455L},
                {0.06, 0.053223829909765978023710895440807027106648031794332L},
                {0.08, 0.071017364833454740933678952589335595973692337768206L},
                {0.1, 0.088855990494257687015737250567791777572052244333197L},
                {0.12, 0.10675135602818441680418415862032366157158114337340L},
                {0.14, 0.12471536794266058347700965205066754588544249041608L},
                {0.16, 0.14276024817854752632491250241937863467781806314348L},
                {0.18, 0.16089859600789130339227268394276111706737629217858L},
                {0.2, 0.17914345462129167649274901662647187030390927701953L},
                {0.22, 0.19750838337227370288060001612909229800725174974097L},
                {0.24, 0.21600753678729464545449221534439871936296488192031L},
                {0.26, 0.23465575162492161076204889728668819204447671512447L},
                {0.28, 0.25346864348386572929532241400293445878523706708597L},
                {0.3, 0.27246271472675435562195759858756581266755846463101L},
                {0.32, 0.29165547581744204898195789816256667311916049007978L},
                {0.34, 0.31106558258078476766865874216382214004176371527047L},
                {0.36, 0.33071299240667360702855365239202747308739782520415L},
                {0.38, 0.35061914306308926506738192881390722874950940764075L},
                {0.4, 0.37080715859355792905824947752244913860430488316293L},
                {0.42, 0.39130208780283210702901160177202265983461830797914L},
                {0.44, 0.41213118214846543465371769260873918822421955427814L},
                {0.46, 0.43332422154706794135044570463490144251715229096232L},
                {0.48, 0.45491389879854004785364155852191412030024257150960L},
                {0.5, 0.47693627620446987338141835364313055980896974905947L},
                {0.52, 0.49943133175366345326164267598535422011131753247479L},
                {0.54, 0.52244361731717893285352929339417012045078590470794L},
                {0.56, 0.54602305813905509637966116314198870157785666155699L},
                {0.58, 0.57022593225950956936474072279649840144935403988631L},
                {0.6, 0.59511608144999485001930036016810825343961688627985L},
                {0.62, 0.62076642340926986269470763094283973396910466246265L},
                {0.64, 0.64726086087507356778569552752351260307199805216753L},
                {0.66, 0.67469672087225281689920346403004819361781262552046L},
                {0.68, 0.70318791282203616647863856959245649687698632873061L},
                {0.7, 0.73286907795921685221881746105801553557176747076776L},
                {0.72, 0.76390113173723643622162735649883701362040687681414L},
                {0.74, 0.79647880561170738536314283162310242412874769726486L},
                {0.76, 0.83084112847456012056939397503022302181762116635478L},
                {0.78, 0.86728635099387474053932140772705022745846114980739L},
                {0.8, 0.90619380243682322007116270309566286665086687474622L},
                {0.82, 0.94805697623234998774463106326994728220647435246924L},
                {0.84, 0.99353562834730426111305935025233398481497421350394L},
                {0.86, 1.0435418436397588726712998682285193875903823653292L},
                {0.88, 1.0993909519492192652447347060305922130956112866969L},
                {0.9, 1.1630871536766740867262542605629475934779325500021L},
                {0.92, 1.2379219927112447060181604356171291766640827873706L},
                {0.94, 1.3299219143360638040159346045180463526600932312927L},
                {0.96, 1.4522197815622468501434208635071913776081156823245L},
                {0.98, 1.6449763571331870501772034352495116246653430362888L}};

        if (!std::isnan(erf_inv(T(-2))))
        {
                error("erf_inv(-2) is not NAN");
        }

        if (!std::isnan(erf_inv(T(2))))
        {
                error("erf_inv(2) is not NAN");
        }

        if (!(erf_inv(T(-1)) == -limits<T>::infinity()))
        {
                error("erf_inv(-1) is not -infinity");
        }

        if (!(erf_inv(T(0)) == 0))
        {
                error("erf_inv(0) is not 0");
        }

        if (!(erf_inv(T(1)) == limits<T>::infinity()))
        {
                error("erf_inv(1) is not infinity");
        }

        for (const std::array<T, 2>& v : DATA)
        {
                T e1 = std::abs((erf_inv(v[0]) - v[1]) / v[1]);
                T e2 = std::abs((erf_inv(-v[0]) - (-v[1])) / v[1]);
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
        constexpr int COUNT = 1000000;

        std::mt19937_64 engine = create_engine<std::mt19937_64>();
        std::uniform_real_distribution<T> urd(-1.0001, 1.0001);
        std::vector<T> data(COUNT);
        for (int i = 0; i < COUNT; ++i)
        {
                data[i] = urd(engine);
        }

        std::vector<T> result(COUNT);
        TimePoint time_point = time();
        for (int i = 0; i < COUNT; ++i)
        {
                result[i] = erf_inv(data[i]);
        }
        double duration = duration_from(time_point);

        LOG(std::string("erf_inv<") + type_name<T>() + "> "
            + to_string_digit_groups(std::lround(data.size() / duration)) + " per second");
}

void test_erf()
{
        LOG("Test erf_inv from array");
        test_erf_inv_from_array<float>(0.003);
        test_erf_inv_from_array<double>(0.003);
        test_erf_inv_from_array<long double>(0.003);

        LOG("Test erf_inv");
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

TEST_SMALL("Erf Inverse", test_erf)
TEST_PERFORMANCE("Erf Inverse", test_erf_performance)
}
}
