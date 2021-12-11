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

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/test/test.h>

#include <gmpxx.h>
#include <vector>

namespace ns
{
namespace
{
constexpr int N = 100'000;
constexpr int COUNT = 1000;

template <typename T>
double computation(const std::vector<T>& data)
{
        constexpr T ADD = 20;
        constexpr T SUB = 30;

        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                for (const T& v : data)
                {
                        do_not_optimize((v + ADD) * (v - SUB) + ADD);
                }
        }
        return duration_from(start_time);
}

double computation(const std::vector<mpz_class>& data)
{
        const mpz_class add = 20;
        const mpz_class sub = 30;

        mpz_class tmp1;
        mpz_class tmp2;

        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                for (const mpz_class& v : data)
                {
                        mpz_add(tmp1.get_mpz_t(), v.get_mpz_t(), add.get_mpz_t());
                        mpz_sub(tmp2.get_mpz_t(), v.get_mpz_t(), sub.get_mpz_t());
                        mpz_mul(tmp1.get_mpz_t(), tmp1.get_mpz_t(), tmp2.get_mpz_t());
                        mpz_add(tmp1.get_mpz_t(), tmp1.get_mpz_t(), add.get_mpz_t());
                }
        }
        return duration_from(start_time);
}

double computation(const std::vector<mpf_class>& data)
{
        const mpf_class add = 20;
        const mpf_class sub = 30;

        mpf_class tmp1;
        mpf_class tmp2;

        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < COUNT; ++i)
        {
                for (const mpf_class& v : data)
                {
                        mpf_add(tmp1.get_mpf_t(), v.get_mpf_t(), add.get_mpf_t());
                        mpf_sub(tmp2.get_mpf_t(), v.get_mpf_t(), sub.get_mpf_t());
                        mpf_mul(tmp1.get_mpf_t(), tmp1.get_mpf_t(), tmp2.get_mpf_t());
                        mpf_add(tmp1.get_mpf_t(), tmp1.get_mpf_t(), add.get_mpf_t());
                }
        }
        return duration_from(start_time);
}

template <typename T>
T max_int()
{
        static_assert(std::is_integral_v<T>);
        return std::sqrt(Limits<T>::max()) / 10;
}

class MpfPrecision final
{
        mp_bitcnt_t precision_;

public:
        explicit MpfPrecision(const mp_bitcnt_t& precision) : precision_(mpf_get_default_prec())
        {
                mpf_set_default_prec(precision);
        }

        ~MpfPrecision()
        {
                mpf_set_default_prec(precision_);
        }

        MpfPrecision(const MpfPrecision&) = delete;
        MpfPrecision& operator=(const MpfPrecision&) = delete;
};

template <typename T>
void write(const std::string& name, const std::vector<T>& data)
{
        const long long p = COUNT * (data.size() / computation(data));
        LOG("Compare types <" + name + ">: " + to_string_digit_groups(p) + " o/s");
}

void compare_types()
{
        MpfPrecision mpf_precision(128);

        write("MPZ", std::vector<mpz_class>(N, 1e16));

        write("MPF", std::vector<mpf_class>(N, 1e12));

        write("__float128", std::vector<__float128>(N, 1e12));

        write("float", std::vector<float>(N, 1e6));

        write("double", std::vector<double>(N, 1e12));

        write("long double", std::vector<long double>(N, 1e12));

        write("int", std::vector<int>(N, max_int<int>()));

        write("long", std::vector<long>(N, max_int<long>()));

        write("long long", std::vector<long long>(N, max_int<long long>()));

        write("__int128", std::vector<__int128>(N, 1e16));

        write("unsigned __int128", std::vector<unsigned __int128>(N, 1e16));
}

TEST_PERFORMANCE("Compare arithmetic types", compare_types)
}
}
