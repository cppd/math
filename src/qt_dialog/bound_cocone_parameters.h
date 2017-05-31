/*
Copyright (C) 2017 Topological Manifold

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
#ifndef BOUND_COCONE_PARAMETERS_H
#define BOUND_COCONE_PARAMETERS_H

#include "ui_bound_cocone_parameters.h"

class BoundCoconeParameters final : public QDialog
{
        Q_OBJECT

public:
        explicit BoundCoconeParameters(QWidget* parent = nullptr);
        void set_parameters(int digits, double rho, double alpha);
        void get_parameters(double* rho, double* alpha);

private:
        Ui::BoundCoconeParameters ui;

        double m_rho;
        double m_alpha;

        virtual void done(int r) override;
};

#endif
