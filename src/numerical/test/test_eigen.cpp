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

#include "test_eigen.h"

#include "../eigen.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/type/limit.h>
#include <src/utility/random/engine.h>

#include <random>

namespace numerical
{
namespace
{
bool equal(double a, double b)
{
        return std::abs(a - b) < 1e-8;
}

bool equal(const Vector<3, double>& a, const Vector<3, double>& b)
{
        for (unsigned i = 0; i < 3; ++i)
        {
                if (!equal(a[i], b[i]))
                {
                        return false;
                }
        }
        return true;
}

template <size_t N, typename T>
Matrix<N, N, T> random_symmetric_matrix(T min, T max)
{
        Matrix<N, N, T> m;
        RandomEngineWithSeed<std::mt19937_64> random_engine;
        std::uniform_real_distribution<T> urd(min, max);
        for (unsigned i = 0; i < N; ++i)
        {
                m(i, i) = urd(random_engine);
                for (unsigned j = i + 1; j < N; ++j)
                {
                        T v = urd(random_engine);
                        m(i, j) = v;
                        m(j, i) = v;
                }
        }
        return m;
}

void test_eigen_defined()
{
        constexpr double TOLERANCE = 1e-10;

        Matrix<3, 3, double> a;
        a.row(0) = Vector<3, double>(1.2, 3.4, 5.6);
        a.row(1) = Vector<3, double>(3.4, 7.8, 9.10);
        a.row(2) = Vector<3, double>(5.6, 9.10, 11.12);

        Vector<3, double> eigenvalues;
        std::array<Vector<3, double>, 3> eigenvectors;

        try
        {
                numerical::eigen(a, TOLERANCE, &eigenvalues, &eigenvectors);
        }
        catch (const numerical::EigenException& e)
        {
                error(e.what());
        }

        if (!equal(eigenvalues, Vector<3, double>(-1.453829508, 0.722976163, 20.850853345)))
        {
                error("Eigenvalues error");
        }

        if (!equal(eigenvectors[0], Vector<3, double>(0.831214283, 0.203404459, -0.517406456)))
        {
                error("Eigenvalues error");
        }
        if (!equal(eigenvectors[1], Vector<3, double>(-0.458978533, 0.776240332, -0.432191683)))
        {
                error("Eigenvalues error");
        }
        if (!equal(eigenvectors[2], Vector<3, double>(0.313722043, 0.596722357, 0.738580332)))
        {
                error("Eigenvalues error");
        }
}

template <size_t N, typename T>
void test_eigen_random()
{
        constexpr T TOLERANCE = limits<T>::epsilon() * 100;

        Matrix<N, N, T> matrix = random_symmetric_matrix<N, T>(-1, 1);

        Vector<N, T> eigenvalues;
        std::array<Vector<N, T>, N> eigenvectors;
        try
        {
                numerical::eigen(matrix, TOLERANCE, &eigenvalues, &eigenvectors);
        }
        catch (const numerical::EigenException& e)
        {
                error(e.what());
        }

        T trace = 0;
        T sum = 0;
        T product = 1;
        T det = matrix.determinant();
        for (unsigned i = 0; i < N; ++i)
        {
                trace += matrix(i, i);
                sum += eigenvalues[i];
                product *= eigenvalues[i];
        }
        if (std::abs((trace - sum) / std::max(std::abs(trace), std::abs(sum))) > T(0.01))
        {
                error("Eigenvalues error, trace " + to_string(trace) + " and sum " + to_string(sum) + " are not equal");
        }
        if (std::abs((det - product) / std::max(std::abs(det), std::abs(product))) > T(0.01))
        {
                error("Eigenvalues error, determinant " + to_string(det) + " and product " + to_string(product)
                      + " are not equal");
        }
}

template <typename T>
void test_eigen_random()
{
        test_eigen_random<3, T>();
        test_eigen_random<4, T>();
        test_eigen_random<5, T>();
}
}

void test_eigen()
{
        LOG("Test eigenvalues and eigenvectors");
        test_eigen_defined();
        for (int i = 0; i < 100; ++i)
        {
                test_eigen_random<float>();
                test_eigen_random<double>();
                test_eigen_random<long double>();
        }
        LOG("Test eigenvalues and eigenvectors passed");
}
}
