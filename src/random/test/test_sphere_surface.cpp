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

#include "test_sphere_surface.h"

#include "../sphere_surface.h"
#include "../sphere_uniform.h"

#include <src/com/log.h>
#include <src/com/random/engine.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/numerical/vec.h>

#include <cmath>
#include <random>
#include <sstream>
#include <version>

namespace ns::random
{
namespace
{
#if defined(__cpp_lib_math_special_functions) && __cpp_lib_math_special_functions >= 201603L
long double betal(long double x, long double y)
{
        return std::betal(x, y);
}
#else
#if !defined(__clang__)
#error __cpp_lib_math_special_functions
#endif
long double betal(long double x, long double y)
{
        // Β(x, y) = Γ(x) * Γ(y) / Γ(x + y)
        // Β(x, y) = exp(log(Β(x, y)))
        // Β(x, y) = exp(log(Γ(x) * Γ(y) / Γ(x + y)))
        // Β(x, y) = exp(log(Γ(x)) + log(Γ(y)) - log(Γ(x + y)))
        return std::exp(std::lgamma(x) + std::lgamma(y) - std::lgamma(x + y));
}
#endif

void compare_with_beta(unsigned n)
{
        long double beta = betal(0.5L, (n - 1) / 2.0L) / betal(1.0L, (n - 1) / 2.0L);
        long double function = cosine_sphere_coefficient(n);
        long double discrepancy_percent = std::abs(beta - function) / function * 100;

        if (discrepancy_percent > 1e-10)
        {
                std::ostringstream oss;
                oss << std::fixed;
                oss << std::setprecision(limits<long double>::max_digits10);
                oss << "N = " << n << ": beta = " << beta << ", function = " << function;
                oss << std::scientific;
                oss << std::setprecision(5);
                oss << ", discrepancy = " << discrepancy_percent << "%";

                LOG(oss.str());

                error("Huge discrepancy between beta and function: " + to_string(discrepancy_percent) + "%");
        }
}

void compare_with_beta()
{
        LOG("Compare with beta");

        for (unsigned n = 2; n < 10000; ++n)
        {
                compare_with_beta(n);
        }

        for (unsigned n = 10000; n <= 1'000'000; (n & 1) == 0 ? ++n : n += 999)
        {
                compare_with_beta(n);
        }

        LOG("Check passed");
}

template <std::size_t N, typename T>
void test_cosine()
{
        static_assert(std::is_floating_point_v<T>);

        constexpr int count = 10000000;

        std::mt19937_64 engine = create_engine<std::mt19937_64>();

        long double sum = 0;

        for (int i = 0; i < count; ++i)
        {
                Vector<N, T> v;
                T length_square;
                random_in_sphere(engine, v, length_square);

                // косинус угла между вектором и последней координатной осью
                T c = v[N - 1] / std::sqrt(length_square);

                sum += std::abs(c);
        }

        long double data = static_cast<long double>(count) / sum;
        long double function = cosine_sphere_coefficient(N);
        long double discrepancy_percent = std::abs(data - function) / function * 100;

        std::ostringstream oss;
        oss << std::fixed;
        oss << std::setprecision(limits<long double>::max_digits10);
        oss << std::setw(2) << N << ": data = " << data << ", function = " << function;
        oss << std::setprecision(5);
        oss << ", discrepancy = " << discrepancy_percent << "%";

        LOG(oss.str());

        if (discrepancy_percent > 0.1)
        {
                error("Huge discrepancy between data and function: " + to_string(discrepancy_percent) + "%");
        }
}

template <typename T, std::size_t... I>
void test_cosine(std::integer_sequence<std::size_t, I...>)
{
        (test_cosine<I + 2, T>(), ...);
}

template <typename T>
void test_cosine()
{
        LOG(std::string("Test cosine sphere, ") + type_name<T>());

        test_cosine<T>(std::make_integer_sequence<std::size_t, 10>());

        LOG("Check passed");
}

template <typename T>
void compare(T v1, T v2)
{
        if (!(is_finite(v1) && is_finite(v2) && ((v1 == v2) || std::abs((v1 - v2) / std::max(v1, v2)) < T(0.001))))
        {
                error("Numbers are not equal " + to_string(v1) + " and " + to_string(v2));
        }
}

template <typename T>
void test_area()
{
        LOG(std::string("Test sphere area, ") + type_name<T>());

        compare(sphere_relative_area<2, T>(0.5, 1), T(0.50000000000000000000000000000000000000000000000000L));
        compare(sphere_relative_area<3, T>(0.5, 1), T(0.33728025602223299871534497516085304825933477649182L));
        compare(sphere_relative_area<4, T>(0.5, 1), T(0.23304338949555370281412061392963853923007702233762L));
        compare(sphere_relative_area<5, T>(0.5, 1), T(0.16456605049432905175652851085684561857127023868729L));
        compare(sphere_relative_area<6, T>(0.5, 1), T(0.11847776692887839197760002141640185370388427675061L));
        compare(sphere_relative_area<7, T>(0.5, 1), T(0.086747410598336502855863559308529083473508300192666L));
        compare(sphere_relative_area<8, T>(0.5, 1), T(0.064445032897166510836125417254910295152840007397306L));
        compare(sphere_relative_area<9, T>(0.5, 1), T(0.048475825004558812194932172261776921435799662926282L));
        compare(sphere_relative_area<10, T>(0.5, 1), T(0.036852689606665752354152799788873530801949717378474L));
        compare(sphere_relative_area<11, T>(0.5, 1), T(0.028271142654439652603483734391164058265792744319845L));
        compare(sphere_relative_area<12, T>(0.5, 1), T(0.021856353187699151682891120312318245519917593143986L));
        compare(sphere_relative_area<13, T>(0.5, 1), T(0.017009720583937844245155790468162021432350290550126L));
        compare(sphere_relative_area<14, T>(0.5, 1), T(0.013313970393473262087067334544828366956211559294135L));
        compare(sphere_relative_area<15, T>(0.5, 1), T(0.010473262061717212781929422559521292732168015614157L));

        LOG("Check passed");
}
}

void test_sphere_surface(bool all_tests)
{
        test_area<float>();
        test_area<double>();
        test_area<long double>();

        if (all_tests)
        {
                LOG("");
                compare_with_beta();
                LOG("");
                test_cosine<float>();
                LOG("");
                test_cosine<double>();
        }
}
}
