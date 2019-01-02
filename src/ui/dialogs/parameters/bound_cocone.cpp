/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "bound_cocone.h"

#include "com/error.h"
#include "com/print.h"
#include "ui/dialogs/messages/message_box.h"
#include "ui/support/support.h"

#include <QPointer>
#include <cmath>

namespace bound_cocone_parameters_implementation
{
BoundCoconeParameters::BoundCoconeParameters(QWidget* parent) : QDialog(parent)
{
        ui.setupUi(this);
        setWindowTitle("BoundCocone Parameters");
}

bool BoundCoconeParameters::show(int minimum_rho_exponent, int minimum_alpha_exponent, double* rho, double* alpha)
{
        if (!(-10 <= minimum_rho_exponent && minimum_rho_exponent < 0))
        {
                error(u8"Error BoundCocone minimum ρ exponent: " + to_string(minimum_rho_exponent));
        }
        if (!(-10 <= minimum_alpha_exponent && minimum_alpha_exponent < 0))
        {
                error(u8"Error BoundCocone minimum α exponent: " + to_string(minimum_alpha_exponent));
        }

        double min_rho = std::pow(10, minimum_rho_exponent);
        double max_rho = 1 - min_rho;
        double min_alpha = std::pow(10, minimum_alpha_exponent);
        double max_alpha = 1 - min_alpha;

        if (!(min_rho <= *rho && *rho <= max_rho))
        {
                error(u8"BoundCocone parameter ρ range error: ρ = " + to_string(*rho) + ", range = (" + to_string(min_rho) +
                      ", " + to_string(max_rho) + ")");
        }
        if (!(min_alpha <= *alpha && *alpha <= max_alpha))
        {
                error(u8"BoundCocone parameter α range error: α = " + to_string(*alpha) + ", range = (" + to_string(min_alpha) +
                      ", " + to_string(max_alpha) + ")");
        }

        m_min_rho = min_rho;
        m_max_rho = max_rho;
        m_min_alpha = min_alpha;
        m_max_alpha = max_alpha;

        ui.doubleSpinBox_rho->setDecimals(std::abs(minimum_rho_exponent));
        ui.doubleSpinBox_rho->setMinimum(min_rho);
        ui.doubleSpinBox_rho->setMaximum(max_rho);
        ui.doubleSpinBox_rho->setSingleStep(min_rho);
        ui.doubleSpinBox_rho->setValue(*rho);

        ui.doubleSpinBox_alpha->setDecimals(std::abs(minimum_alpha_exponent));
        ui.doubleSpinBox_alpha->setMinimum(min_alpha);
        ui.doubleSpinBox_alpha->setMaximum(max_alpha);
        ui.doubleSpinBox_alpha->setSingleStep(min_alpha);
        ui.doubleSpinBox_alpha->setValue(*alpha);

        if (QPointer ptr(this); !this->exec() || ptr.isNull())
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

        m_rho = ui.doubleSpinBox_rho->value();
        if (!(m_min_rho <= m_rho && m_rho <= m_max_rho))
        {
                std::string msg = u8"ρ range error (" + to_string(m_min_rho) + ", " + to_string(m_max_rho) + ")";
                dialog::message_critical(this, msg);
                return;
        }

        m_alpha = ui.doubleSpinBox_alpha->value();
        if (!(m_min_alpha <= m_alpha && m_alpha <= m_max_alpha))
        {
                std::string msg = u8"α range error (" + to_string(m_min_alpha) + ", " + to_string(m_max_alpha) + ")";
                dialog::message_critical(this, msg);
                return;
        }

        QDialog::done(r);
}
}

namespace dialog
{
bool bound_cocone_parameters(QWidget* parent, int minimum_rho_exponent, int minimum_alpha_exponent, double* rho, double* alpha)
{
        QtObjectInDynamicMemory<bound_cocone_parameters_implementation::BoundCoconeParameters> w(parent);
        return w->show(minimum_rho_exponent, minimum_alpha_exponent, rho, alpha);
}
}
