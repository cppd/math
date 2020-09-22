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
}

void test_eigen()
{
        LOG("Test eigenvalues and eigenvectors");

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

        LOG("Test eigenvalues and eigenvectors passed");
}
