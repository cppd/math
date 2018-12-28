/*
Copyright (C) 2017, 2018 Topological Manifold

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

/*
 Thomas H. Cormen, Charles E. Leiserson, Ronald L. Rivest, Clifford Stein.
 Introduction to Algorithms. Third Edition.
 The MIT Press, 2009.

 Глава 29 Linear Programming.
*/

#pragma once

#include "com/combinatorics.h"
#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "com/types.h"
#include "com/vec.h"

#include <array>
#include <sstream>

namespace numerical
{
enum class ConstraintSolution
{
        Infeasible,
        Feasible,
        Unbound,
        Cycling
};

inline const char* constraint_solution_to_string(const ConstraintSolution& cs) noexcept
{
        switch (cs)
        {
        case ConstraintSolution::Infeasible:
                return "Infeasible";
        case ConstraintSolution::Feasible:
                return "Feasible";
        case ConstraintSolution::Unbound:
                return "Unbound";
        case ConstraintSolution::Cycling:
                return "Cycling";
        }
        error_fatal("Unknown ConstraintSolution");
}

namespace simplex_algorithm_implementation
{
template <size_t N, size_t M, typename T>
void print_simplex_algorithm_data(const std::array<T, M>& b, const std::array<Vector<N, T>, M>& a, const T& v,
                                  const Vector<N, T>& c) noexcept
{
        try
        {
                static_assert(std::is_floating_point_v<T>);

                LOG("z = " + to_string(v) + " + " + to_string(c));
                for (unsigned i = 0; i < M; ++i)
                {
                        LOG(to_string(b[i]) + " + " + to_string(a[i]));
                }
        }
        catch (...)
        {
                error_fatal("Error in print simplex algorithm data function 1");
        }
}

template <size_t N, size_t M, typename T>
void print_simplex_algorithm_data(const std::array<T, M>& b, const std::array<Vector<N, T>, M>& a, const T& v,
                                  const Vector<N, T>& c, const std::array<unsigned, N>& map_n,
                                  const std::array<unsigned, M>& map_m) noexcept
{
        static_assert(std::is_floating_point_v<T>);
        try
        {
                int int_w = (N + M - 1) < 10 ? 1 : std::floor(std::log10(N + M - 1)) + 1;

                int precision = std::numeric_limits<T>::max_digits10;

                std::ostringstream oss;
                oss << std::setprecision(precision);

                int float_w = precision + 6 + 3;

                oss << std::setfill('-');
                oss << std::setw(float_w + 4 + int_w) << "b(v)";
                for (unsigned n = 0; n < N; ++n)
                {
                        oss << std::setw(float_w - int_w - 1) << "[" << std::setw(int_w) << map_n[n] << "]";
                }
                oss << std::setfill(' ');
                oss << '\n';

                oss << "z = " << std::setw(int_w) << " ";
                oss << std::setw(float_w) << v;
                for (unsigned n = 0; n < N; ++n)
                {
                        oss << std::setw(float_w) << c[n];
                }

                oss << "\n";
                oss << "---";

                for (unsigned m = 0; m < M; ++m)
                {
                        oss << '\n';
                        oss << "[" << std::setw(int_w) << map_m[m] << "]: ";
                        oss << std::setw(float_w) << b[m];
                        for (unsigned n = 0; n < N; ++n)
                        {
                                oss << std::setw(float_w) << a[m][n];
                        }
                }

                LOG(oss.str());
        }
        catch (...)
        {
                error_fatal("Error in print simplex algorithm data function 2");
        }
}

// 29.3 The simplex algorithm.
// Pivoting.
template <size_t N, size_t M, typename T>
void pivot(std::array<T, M>& b, std::array<Vector<N, T>, M>& a, T& v, Vector<N, T>& c, unsigned l, unsigned e) noexcept
{
        static_assert(is_native_floating_point<T>);

        ASSERT(l < M);
        ASSERT(e < N);
        ASSERT(a[l][e] != 0);

        // С отличием от алгоритма в книге в том, что пишется в переменные
        // с теми же номерами без замены индексов l и e, а также используется
        // другая работа со знаками.

        b[l] = -b[l] / a[l][e];
        for (unsigned j = 0; j < N; ++j)
        {
                if (j == e)
                {
                        continue;
                }
                a[l][j] = -a[l][j] / a[l][e];
        }
        a[l][e] = 1 / a[l][e];

        for (unsigned i = 0; i < M; ++i)
        {
                if (i == l)
                {
                        continue;
                }
                b[i] = b[i] + a[i][e] * b[l];
                for (unsigned j = 0; j < N; ++j)
                {
                        if (j == e)
                        {
                                continue;
                        }
                        a[i][j] = a[i][j] + a[i][e] * a[l][j];
                }
                a[i][e] = a[i][e] * a[l][e];
        }

        v = v + c[e] * b[l];
        for (unsigned j = 0; j < N; ++j)
        {
                if (j == e)
                {
                        continue;
                }
                c[j] = c[j] + c[e] * a[l][j];
        }
        c[e] = c[e] * a[l][e];
}

template <size_t N_Source, size_t M, typename T>
void make_aux_and_maps(const std::array<Vector<N_Source, T>, M>& a_input, std::array<T, M>* b,
                       std::array<Vector<N_Source + 1, T>, M>* a, T* v, Vector<N_Source + 1, T>* c,
                       std::array<unsigned, N_Source + 1>* map_n, std::array<unsigned, M>* map_m) noexcept
{
        for (unsigned m = 0; m < M; ++m)
        {
                T max = std::abs(a_input[m][0]);
                for (unsigned n = 1; n < N_Source; ++n)
                {
                        max = std::max(max, std::abs(a_input[m][n]));
                }

                max = (max != 0) ? max : 1;

                T max_reciprocal = 1 / max;

                (*b)[m] *= max_reciprocal;
                (*a)[m][0] = 1;
                for (unsigned n = 0; n < N_Source; ++n)
                {
                        (*a)[m][n + 1] = a_input[m][n] * max_reciprocal;
                }
        }

        //

        constexpr unsigned N = N_Source + 1;

        //

        *v = 0;

        (*c)[0] = -1;
        for (unsigned n = 1; n < N; ++n)
        {
                (*c)[n] = 0;
        }

        //

        for (unsigned i = 0; i < N; ++i)
        {
                (*map_n)[i] = i;
        }
        for (unsigned i = 0; i < M; ++i)
        {
                (*map_m)[i] = i + N;
        }
}

template <size_t N, size_t M, typename T>
bool variable_x0_is_zero(const std::array<T, M>& b, const std::array<unsigned, N>& map_n,
                         const std::array<unsigned, M>& map_m) noexcept
{
        for (unsigned n = 0; n < N; ++n)
        {
                if (map_n[n] == 0)
                {
                        return true;
                }
        }

        for (unsigned m = 0; m < M; ++m)
        {
                // b из-за численных ошибок может быть меньше 0, поэтому <= 0 вместо == 0
                if (map_m[m] == 0 && b[m] <= 0)
                {
                        return true;
                }
        }

        return false;
}

template <size_t N, typename T>
bool find_positive_index(const Vector<N, T>& c, unsigned* e) noexcept
{
        T max_abs_c = std::abs(c[0]);
        for (unsigned i = 1; i < N; ++i)
        {
                max_abs_c = std::max(max_abs_c, std::abs(c[i]));
        }
        T eps_c = max_abs_c * (2 * limits<T>::epsilon());

        for (unsigned i = 0; i < N; ++i)
        {
                if (c[i] > eps_c)
                {
                        *e = i;
                        return true;
                }
        }

        return false;
}

// 29.5 The initial basic feasible solution.
// Finding an initial solution.
// Упрощённый вариант INITIALIZE-SIMPLEX для определения
// наличия решения системы неравенств.
template <bool with_print, size_t N_Source, size_t M, typename T>
ConstraintSolution solve_constraints(std::array<T, M> b, const std::array<Vector<N_Source, T>, M>& a_input) noexcept
{
        static_assert(std::is_floating_point_v<T> || (!with_print && is_native_floating_point<T>));
        static_assert(N_Source > 0 && M > 0);

        constexpr unsigned N = N_Source + 1;

        constexpr int MAX_ITERATION_COUNT = binomial(N + M, M);

        //

        T min = b[0];
        for (unsigned m = 1; m < M; ++m)
        {
                min = std::min(b[m], min);
        }
        if (min >= 0)
        {
                return ConstraintSolution::Feasible;
        }

        //

        T v;
        Vector<N, T> c;
        std::array<Vector<N, T>, M> a;

        std::array<unsigned, N> map_n;
        std::array<unsigned, M> map_m;

        make_aux_and_maps(a_input, &b, &a, &v, &c, &map_n, &map_m);

        if constexpr (with_print)
        {
                LOG("");
                LOG("Preprocessed");
                print_simplex_algorithm_data(b, a, v, c, map_n, map_m);
        }

        //

        min = b[0];
        unsigned k = 0;
        for (unsigned m = 1; m < M; ++m)
        {
                if (b[m] < min)
                {
                        min = b[m];
                        k = m;
                }
        }
        if (min >= 0)
        {
                return ConstraintSolution::Feasible;
        }

        pivot(b, a, v, c, k, 0);
        std::swap(map_m[k], map_n[0]);

        if constexpr (with_print)
        {
                LOG("");
                LOG("First pivot");
                print_simplex_algorithm_data(b, a, v, c, map_n, map_m);
        }

        //

        // Рассматривается со второго варианта, поэтому начинается со второй итерации
        for (int iteration = 2;; ++iteration)
        {
                unsigned e;

                if (!find_positive_index(c, &e))
                {
                        // Все индексы меньше или равны 0, значит найдено оптимальное решение
                        break;
                }

                if (iteration >= MAX_ITERATION_COUNT)
                {
                        return ConstraintSolution::Cycling;
                }

                T max_delta = limits<T>::lowest();
                unsigned l = limits<unsigned>::max();
                static_assert(M > 0 && M - 1 < limits<unsigned>::max());
                for (unsigned i = 0; i < M; ++i)
                {
                        // В теории не должно становиться меньше 0, но из-за численных
                        // ошибок может, поэтому поставить значение больше или равно 0.
                        b[i] = std::max(static_cast<T>(0), b[i]);

                        if (a[i][e] < 0)
                        {
                                T delta = b[i] / a[i][e];

                                if (delta > max_delta || l == limits<unsigned>::max())
                                {
                                        max_delta = delta;
                                        l = i;
                                }
                        }
                }

                if (l == limits<unsigned>::max())
                {
                        return ConstraintSolution::Unbound;
                }

                pivot(b, a, v, c, l, e);
                std::swap(map_m[l], map_n[e]);

                if constexpr (with_print)
                {
                        LOG("");
                        LOG("iteration " + to_string(iteration));
                        print_simplex_algorithm_data(b, a, v, c, map_n, map_m);
                }
        }

        if (variable_x0_is_zero(b, map_n, map_m))
        {
                return ConstraintSolution::Feasible;
        }

        return ConstraintSolution::Infeasible;
}
}

template <size_t N, size_t M, typename T>
ConstraintSolution solve_constraints(const std::array<Vector<N, T>, M>& a, const std::array<T, M>& b) noexcept
{
        return simplex_algorithm_implementation::solve_constraints<false>(b, a);
}

template <size_t N, size_t M, typename T>
ConstraintSolution solve_constraints_with_print(const std::array<Vector<N, T>, M>& a, const std::array<T, M>& b) noexcept
{
        return simplex_algorithm_implementation::solve_constraints<true>(b, a);
}
}
