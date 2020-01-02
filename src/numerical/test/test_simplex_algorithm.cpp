/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "test_simplex_algorithm.h"

#include "com/log.h"
#include "com/print.h"
#include "com/type/name.h"
#include "numerical/simplex.h"

namespace
{
template <typename T>
void test_pivot()
{
        namespace impl = numerical::simplex_algorithm_implementation;

        LOG(std::string("PIVOT, ") + type_name<T>());

        std::array<T, 3> b{30, 24, 36};
        std::array<Vector<3, T>, 3> a{Vector<3, T>(-1, -1, -3), Vector<3, T>(-2, -2, -5), Vector<3, T>(-4, -1, -2)};
        T v = 5;
        Vector<3, T> c(3, 1, 2);

        impl::pivot(b, a, v, c, 2, 0);

        if (!(b == std::array<T, 3>{21, 6, 9}))
        {
                impl::print_simplex_algorithm_data(b, a, v, c);
                error("b error");
        }

        if (!(a == std::array<Vector<3, T>, 3>{Vector<3, T>(0.25, -0.75, -2.5), Vector<3, T>(0.5, -1.5, -4),
                                               Vector<3, T>(-0.25, -0.25, -0.5)}))
        {
                impl::print_simplex_algorithm_data(b, a, v, c);
                error("a error");
        }

        if (!(v == 32))
        {
                impl::print_simplex_algorithm_data(b, a, v, c);
                error("v error");
        }

        if (!(c == Vector<3, T>(-0.75, 0.25, 0.5)))
        {
                impl::print_simplex_algorithm_data(b, a, v, c);
                error("c error");
        }

        LOG("passed");
}

template <typename T>
void test_feasible()
{
        namespace n = numerical;

        LOG(std::string("SOLVE CONSTRAINTS, ") + type_name<T>());

        {
                std::array<T, 2> b{2, -4};
                std::array<Vector<2, T>, 2> a{Vector<2, T>(-2, 1), Vector<2, T>(-1, 5)};

                n::ConstraintSolution cs = n::solve_constraints(a, b);
                if (cs != n::ConstraintSolution::Feasible)
                {
                        n::solve_constraints_with_print(a, b);
                        LOG(n::constraint_solution_to_string(cs));
                        error("Not Feasible");
                }
                LOG("passed feasible");
        }
        {
                std::array<T, 5> b{-1.23456, 3.12321, -1.14321, 3.32123, -4.3214e10};
                std::array<Vector<2, T>, 5> a{Vector<2, T>(1, 0), Vector<2, T>(-1, 0), Vector<2, T>(0, 1), Vector<2, T>(0, -1),
                                              Vector<2, T>(1.01e10, 1.00132e10)};

                n::ConstraintSolution cs = n::solve_constraints(a, b);
                if (cs != n::ConstraintSolution::Feasible)
                {
                        n::solve_constraints_with_print(a, b);
                        LOG(n::constraint_solution_to_string(cs));
                        error("Not Feasible");
                }
                LOG("passed feasible");
        }
        {
                std::array<T, 5> b{-1.23456, -3.12321, -1.14321, 3.32123, -4.3214};
                std::array<Vector<2, T>, 5> a{Vector<2, T>(1, 0), Vector<2, T>(-1, 0), Vector<2, T>(0, 1), Vector<2, T>(0, -1),
                                              Vector<2, T>(1.01, 1.00132)};

                n::ConstraintSolution cs = n::solve_constraints(a, b);
                if (cs != n::ConstraintSolution::Infeasible)
                {
                        n::solve_constraints_with_print(a, b);
                        LOG(n::constraint_solution_to_string(cs));
                        error("Not Infeasible");
                }
                LOG("passed infeasible");
        }
}

void test_pivot()
{
        test_pivot<float>();
        LOG("");
        test_pivot<double>();
        LOG("");
        test_pivot<long double>();
}

void test_feasible()
{
        test_feasible<float>();
        LOG("");
        test_feasible<double>();
        LOG("");
        test_feasible<long double>();
}
}

void test_simplex_algorithm()
{
        test_pivot();
        LOG("");
        test_feasible();
}
