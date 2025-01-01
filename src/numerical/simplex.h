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

/*
Thomas H. Cormen, Charles E. Leiserson, Ronald L. Rivest, Clifford Stein.
Introduction to Algorithms. Third Edition.
The MIT Press, 2009.

29. Linear Programming
*/

#pragma once

#include "vector.h"

#include <src/com/combinatorics.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/type/concept.h>
#include <src/com/type/limit.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string_view>

namespace ns::numerical
{
enum class ConstraintSolution
{
        INFEASIBLE,
        FEASIBLE,
        UNBOUND,
        CYCLING
};

inline const char* constraint_solution_to_string(const ConstraintSolution cs)
{
        switch (cs)
        {
        case ConstraintSolution::INFEASIBLE:
                return "Infeasible";
        case ConstraintSolution::FEASIBLE:
                return "Feasible";
        case ConstraintSolution::UNBOUND:
                return "Unbound";
        case ConstraintSolution::CYCLING:
                return "Cycling";
        }
        error_fatal("Unknown ConstraintSolution");
}

namespace simplex_algorithm_implementation
{
template <std::size_t N, std::size_t M, typename T>
void print_simplex_algorithm_data_impl(
        const std::string_view text,
        const std::array<T, M>& b,
        const std::array<Vector<N, T>, M>& a,
        const T& v,
        const Vector<N, T>& c,
        const std::array<unsigned, N>& map_n,
        const std::array<unsigned, M>& map_m)
{
        std::ostringstream oss;

        oss << '\n';
        oss << text << '\n';

        const int int_w = (N + M - 1) < 10 ? 1 : std::floor(std::log10(N + M - 1)) + 1;

        const int precision = Limits<T>::max_digits10();

        oss << std::setprecision(precision);

        const int float_w = precision + 6 + 3;

        oss << std::setfill('-');
        oss << std::setw(float_w + 4 + int_w) << "b(v)";
        for (std::size_t n = 0; n < N; ++n)
        {
                oss << std::setw(float_w - int_w - 1) << "[" << std::setw(int_w) << map_n[n] << "]";
        }
        oss << std::setfill(' ');
        oss << '\n';

        oss << "z = " << std::setw(int_w) << " ";
        oss << std::setw(float_w) << v;
        for (std::size_t n = 0; n < N; ++n)
        {
                oss << std::setw(float_w) << c[n];
        }

        oss << "\n";
        oss << "---";

        for (std::size_t m = 0; m < M; ++m)
        {
                oss << '\n';
                oss << "[" << std::setw(int_w) << map_m[m] << "]: ";
                oss << std::setw(float_w) << b[m];
                for (std::size_t n = 0; n < N; ++n)
                {
                        oss << std::setw(float_w) << a[m][n];
                }
        }

        LOG(oss.str());
}

template <std::size_t N, std::size_t M, typename T>
void print_simplex_algorithm_data(
        const std::string_view text,
        const std::array<T, M>& b,
        const std::array<Vector<N, T>, M>& a,
        const T& v,
        const Vector<N, T>& c,
        const std::array<unsigned, N>& map_n,
        const std::array<unsigned, M>& map_m) noexcept
{
        static_assert(std::is_floating_point_v<T>);
        try
        {
                print_simplex_algorithm_data_impl(text, b, a, v, c, map_n, map_m);
        }
        catch (...)
        {
                error_fatal("Error in print simplex algorithm data function");
        }
}

// 29.3 The simplex algorithm.
// Pivoting.
// Compute the coefficients of the equation for new basic variable.
// Lines 3–6 of PIVOT.
template <std::size_t N, std::size_t M, typename T>
void pivot_equation_coefficients(
        std::array<T, M>& b,
        std::array<Vector<N, T>, M>& a,
        const std::size_t l,
        const std::size_t e)
{
        b[l] = -b[l] / a[l][e];
        for (std::size_t j = 0; j < N; ++j)
        {
                if (j == e)
                {
                        continue;
                }
                a[l][j] = -a[l][j] / a[l][e];
        }
        a[l][e] = 1 / a[l][e];
}

// 29.3 The simplex algorithm.
// Pivoting.
// Compute the coefficients of the remaining constraints.
// Lines 8–12 of PIVOT.
template <std::size_t N, std::size_t M, typename T>
void pivot_constraint_coefficients(
        std::array<T, M>& b,
        std::array<Vector<N, T>, M>& a,
        const std::size_t l,
        const std::size_t e)
{
        for (std::size_t i = 0; i < M; ++i)
        {
                if (i == l)
                {
                        continue;
                }
                b[i] = b[i] + a[i][e] * b[l];
                for (std::size_t j = 0; j < N; ++j)
                {
                        if (j == e)
                        {
                                continue;
                        }
                        a[i][j] = a[i][j] + a[i][e] * a[l][j];
                }
                a[i][e] = a[i][e] * a[l][e];
        }
}

// 29.3 The simplex algorithm.
// Pivoting.
// Compute the objective function.
// Lines 14–17 of PIVOT.
template <std::size_t N, std::size_t M, typename T>
void pivot_objective_function(
        const std::array<T, M>& b,
        const std::array<Vector<N, T>, M>& a,
        T& v,
        Vector<N, T>& c,
        const std::size_t l,
        const std::size_t e)
{
        v = v + c[e] * b[l];
        for (std::size_t j = 0; j < N; ++j)
        {
                if (j == e)
                {
                        continue;
                }
                c[j] = c[j] + c[e] * a[l][j];
        }
        c[e] = c[e] * a[l][e];
}

// 29.3 The simplex algorithm.
// Pivoting.
template <std::size_t N, std::size_t M, typename T>
void pivot(
        std::array<T, M>& b,
        std::array<Vector<N, T>, M>& a,
        T& v,
        Vector<N, T>& c,
        const std::size_t l,
        const std::size_t e)
{
        static_assert(FloatingPoint<T>);

        ASSERT(l < M);
        ASSERT(e < N);
        ASSERT(a[l][e] != 0);

        pivot_equation_coefficients(b, a, l, e);
        pivot_constraint_coefficients(b, a, l, e);
        pivot_objective_function(b, a, v, c, l, e);
}

template <std::size_t N_SOURCE, std::size_t M, typename T>
void make_aux_and_maps(
        const std::array<Vector<N_SOURCE, T>, M>& a_input,
        std::array<T, M>* const b,
        std::array<Vector<N_SOURCE + 1, T>, M>* const a,
        T* const v,
        Vector<N_SOURCE + 1, T>* const c,
        std::array<unsigned, N_SOURCE + 1>* const map_n,
        std::array<unsigned, M>* const map_m)
{
        for (std::size_t m = 0; m < M; ++m)
        {
                T max = std::abs(a_input[m][0]);
                for (std::size_t n = 1; n < N_SOURCE; ++n)
                {
                        max = std::max(max, std::abs(a_input[m][n]));
                }

                max = (max != 0) ? max : 1;

                const T max_reciprocal = 1 / max;

                (*b)[m] *= max_reciprocal;
                (*a)[m][0] = 1;
                for (std::size_t n = 0; n < N_SOURCE; ++n)
                {
                        (*a)[m][n + 1] = a_input[m][n] * max_reciprocal;
                }
        }

        //

        static constexpr std::size_t N = N_SOURCE + 1;

        //

        *v = 0;

        (*c)[0] = -1;
        for (std::size_t i = 1; i < N; ++i)
        {
                (*c)[i] = 0;
        }

        //

        for (std::size_t i = 0; i < N; ++i)
        {
                (*map_n)[i] = i;
        }
        for (std::size_t i = 0; i < M; ++i)
        {
                (*map_m)[i] = i + N;
        }
}

template <std::size_t N, std::size_t M, typename T>
bool variable_x0_is_zero(
        const std::array<T, M>& b,
        const std::array<unsigned, N>& map_n,
        const std::array<unsigned, M>& map_m)
{
        for (std::size_t n = 0; n < N; ++n)
        {
                if (map_n[n] == 0)
                {
                        return true;
                }
        }

        for (std::size_t m = 0; m < M; ++m)
        {
                if (map_m[m] == 0 && b[m] <= 0)
                {
                        return true;
                }
        }

        return false;
}

template <std::size_t N, typename T>
std::optional<std::size_t> find_positive_index(const Vector<N, T>& c)
{
        const T max_abs_c = [&]
        {
                T res = std::abs(c[0]);
                for (std::size_t i = 1; i < N; ++i)
                {
                        res = std::max(res, std::abs(c[i]));
                }
                return res;
        }();

        const T eps_c = max_abs_c * (2 * Limits<T>::epsilon());

        for (std::size_t i = 0; i < N; ++i)
        {
                if (c[i] > eps_c)
                {
                        return i;
                }
        }

        return std::nullopt;
}

template <std::size_t M, typename T>
bool min_is_non_negative(const std::array<T, M>& b)
{
        for (std::size_t m = 0; m < M; ++m)
        {
                if (!(b[m] >= 0))
                {
                        return false;
                }
        }
        return true;
}

template <std::size_t M, typename T>
std::size_t find_index_of_min(const std::array<T, M>& b)
{
        T min = b[0];
        std::size_t k = 0;
        for (std::size_t m = 1; m < M; ++m)
        {
                if (b[m] < min)
                {
                        min = b[m];
                        k = m;
                }
        }
        return k;
}

// 29.3 The simplex algorithm.
// The formal simplex algorithm.
// Lines 5–12 of SIMPLEX.
template <std::size_t N, std::size_t M, typename T>
std::optional<ConstraintSolution> simplex_iteration(
        const std::size_t e,
        std::array<Vector<N, T>, M>& a,
        std::array<T, M>& b,
        Vector<N, T>& c,
        T& v,
        std::array<unsigned, N>& map_n,
        std::array<unsigned, M>& map_m)
{
        static_assert(M > 0);

        T max_delta = Limits<T>::lowest();
        std::optional<std::size_t> l;

        for (std::size_t i = 0; i < M; ++i)
        {
                b[i] = std::max(static_cast<T>(0), b[i]);

                if (a[i][e] < 0)
                {
                        const T delta = b[i] / a[i][e];

                        if (delta > max_delta || !l)
                        {
                                max_delta = delta;
                                l = i;
                        }
                }
        }

        if (!l)
        {
                return ConstraintSolution::UNBOUND;
        }

        pivot(b, a, v, c, *l, e);
        std::swap(map_m[*l], map_n[e]);

        return std::nullopt;
}

// 29.3 The simplex algorithm.
// The formal simplex algorithm.
// Lines 3–12 of SIMPLEX.
template <bool WITH_PRINT, std::size_t N, std::size_t M, typename T>
std::optional<ConstraintSolution> simplex_iterations(
        std::array<Vector<N, T>, M>& a,
        std::array<T, M>& b,
        Vector<N, T>& c,
        T& v,
        std::array<unsigned, N>& map_n,
        std::array<unsigned, M>& map_m)
{
        static constexpr int MAX_ITERATION_COUNT = BINOMIAL<N + M, M>;

        for (int i = 2;; ++i)
        {
                const auto e = find_positive_index(c);
                if (!e)
                {
                        return std::nullopt;
                }

                if (i >= MAX_ITERATION_COUNT)
                {
                        return ConstraintSolution::CYCLING;
                }

                if (const auto res = simplex_iteration(*e, a, b, c, v, map_n, map_m))
                {
                        return res;
                }

                if constexpr (WITH_PRINT)
                {
                        print_simplex_algorithm_data("iteration " + to_string(i), b, a, v, c, map_n, map_m);
                }
        }
}

// 29.5 The initial basic feasible solution.
// Finding an initial solution.
template <bool WITH_PRINT, std::size_t N_SOURCE, std::size_t M, typename T>
ConstraintSolution solve_constraints(std::array<T, M> b, const std::array<Vector<N_SOURCE, T>, M>& a_input)
{
        static_assert(std::is_floating_point_v<T> || (!WITH_PRINT && FloatingPoint<T>));
        static_assert(N_SOURCE > 0 && M > 0);

        if (min_is_non_negative(b))
        {
                return ConstraintSolution::FEASIBLE;
        }

        //

        static constexpr std::size_t N = N_SOURCE + 1;

        T v;
        Vector<N, T> c;
        std::array<Vector<N, T>, M> a;

        std::array<unsigned, N> map_n;
        std::array<unsigned, M> map_m;

        make_aux_and_maps(a_input, &b, &a, &v, &c, &map_n, &map_m);

        if constexpr (WITH_PRINT)
        {
                print_simplex_algorithm_data("Preprocessed", b, a, v, c, map_n, map_m);
        }

        //

        const std::size_t k = find_index_of_min(b);
        if (b[k] >= 0)
        {
                return ConstraintSolution::FEASIBLE;
        }

        pivot(b, a, v, c, k, 0);
        std::swap(map_m[k], map_n[0]);

        if constexpr (WITH_PRINT)
        {
                print_simplex_algorithm_data("First pivot", b, a, v, c, map_n, map_m);
        }

        //

        const std::optional<ConstraintSolution> simplex_result =
                simplex_iterations<WITH_PRINT>(a, b, c, v, map_n, map_m);

        if (simplex_result)
        {
                return *simplex_result;
        }

        if (variable_x0_is_zero(b, map_n, map_m))
        {
                return ConstraintSolution::FEASIBLE;
        }

        return ConstraintSolution::INFEASIBLE;
}
}

template <std::size_t N, std::size_t M, typename T>
ConstraintSolution solve_constraints(const std::array<Vector<N, T>, M>& a, const std::array<T, M>& b)
{
        return simplex_algorithm_implementation::solve_constraints<false>(b, a);
}

template <std::size_t N, std::size_t M, typename T>
ConstraintSolution solve_constraints_with_print(const std::array<Vector<N, T>, M>& a, const std::array<T, M>& b)
{
        return simplex_algorithm_implementation::solve_constraints<true>(b, a);
}
}
