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

#pragma once

#include "ui_bound_cocone.h"

namespace bound_cocone_parameters_implementation
{
class BoundCoconeParameters final : public QDialog
{
        Q_OBJECT

public:
        explicit BoundCoconeParameters(QWidget* parent = nullptr);

        [[nodiscard]] bool show(int minimum_rho_exponent, int minimum_alpha_exponent, double* rho, double* alpha);

private:
        Ui::BoundCoconeParameters ui;

        double m_min_rho;
        double m_max_rho;
        double m_min_alpha;
        double m_max_alpha;

        double m_rho;
        double m_alpha;

        void done(int r) override;
};
}

[[nodiscard]] bool bound_cocone_parameters(QWidget* parent, int minimum_rho_exponent, int minimum_alpha_exponent, double* rho,
                                           double* alpha);
