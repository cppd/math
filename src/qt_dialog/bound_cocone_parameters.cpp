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

#include "message_box.h"

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

        setWindowTitle("BOUND COCONE parameters");
}

bool BoundCoconeParameters::show(int digits, double* rho, double* alpha)
{
        ui.LineEdit_Rho->setText(to_string_fixed(*rho, digits).c_str());
        ui.LineEdit_Alpha->setText(to_string_fixed(*alpha, digits).c_str());

        if (!this->exec())
        {
                return false;
        }

        *rho = m_rho;
        *alpha = m_alpha;

        return true;
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
                message_critical(this, u8"ρ error");
                return;
        }
        if (!(m_rho > RHO_MIN && m_rho < RHO_MAX))
        {
                QString rho_range = "(" + QString(to_string_fixed(RHO_MIN, RANGE_STR_DIGITS).c_str()) + ", " +
                                    QString(to_string_fixed(RHO_MAX, RANGE_STR_DIGITS).c_str()) + ")";
                message_critical(this, u8"ρ range error " + rho_range);
                return;
        }

        m_alpha = ui.LineEdit_Alpha->text().toDouble(&ok);
        if (!ok)
        {
                message_critical(this, u8"α error");
                return;
        }
        if (!(m_alpha > ALPHA_MIN && m_alpha < ALPHA_MAX))
        {
                QString alpha_range = "(" + QString(to_string_fixed(ALPHA_MIN, RANGE_STR_DIGITS).c_str()) + ", " +
                                      QString(to_string_fixed(ALPHA_MAX, RANGE_STR_DIGITS).c_str()) + ")";
                message_critical(this, u8"α range error " + alpha_range);
                return;
        }

        QDialog::done(r);
}
