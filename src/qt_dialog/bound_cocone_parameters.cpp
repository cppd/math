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

#include "bound_cocone_parameters.h"

#include "com/log.h"
#include "com/print.h"

#include <QtWidgets>

constexpr double RHO_MIN = 1e-3;
constexpr double RHO_MAX = 1;
constexpr double ALPHA_MIN = 1e-3;
constexpr double ALPHA_MAX = 1;

constexpr int RANGE_STR_DIGITS = 10;

BoundCoconeParameters::BoundCoconeParameters(QWidget* parent) : QDialog(parent)
{
        ui.setupUi(this);

        ui.LineEdit_Rho->setValidator(new QDoubleValidator(this));
        ui.LineEdit_Alpha->setValidator(new QDoubleValidator(this));
}

void BoundCoconeParameters::set_parameters(int digits, double rho, double alpha)
{
        ui.LineEdit_Rho->setText(to_string(rho, digits).c_str());
        ui.LineEdit_Alpha->setText(to_string(alpha, digits).c_str());
}

void BoundCoconeParameters::get_parameters(double* rho, double* alpha)
{
        *rho = m_rho;
        *alpha = m_alpha;
}

void BoundCoconeParameters::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        bool ok;

        m_rho = ui.LineEdit_Rho->text().toDouble(&ok);
        if (!ok)
        {
                QMessageBox::critical(this, "Error", "ρ error");
                return;
        }
        if (!(m_rho > RHO_MIN && m_rho < RHO_MAX))
        {
                std::string rho_range =
                        "(" + to_string(RHO_MIN, RANGE_STR_DIGITS) + ", " + to_string(RHO_MAX, RANGE_STR_DIGITS) + ")";
                QMessageBox::critical(this, "Error", "ρ range error " + QString(rho_range.c_str()));
                return;
        }

        m_alpha = ui.LineEdit_Alpha->text().toDouble(&ok);
        if (!ok)
        {
                QMessageBox::critical(this, "Error", "α error");
                return;
        }
        if (!(m_alpha > ALPHA_MIN && m_alpha < ALPHA_MAX))
        {
                std::string alpha_range =
                        "(" + to_string(ALPHA_MIN, RANGE_STR_DIGITS) + ", " + to_string(ALPHA_MAX, RANGE_STR_DIGITS) + ")";
                QMessageBox::critical(this, "Error", "α range error " + QString(alpha_range.c_str()));
                return;
        }

        QDialog::done(r);
}
