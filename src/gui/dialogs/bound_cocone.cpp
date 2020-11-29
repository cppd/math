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

#include "bound_cocone.h"

#include "message.h"

#include "../com/support.h"

#include <src/com/error.h>
#include <src/com/print.h>

#include <cmath>
#include <mutex>

namespace gui::dialog
{
namespace
{
constexpr int MINIMUM_RHO_EXPONENT = -3;
constexpr int MINIMUM_ALPHA_EXPONENT = -3;

std::mutex g_mutex;
BoundCoconeParameters g_parameters{.rho = 0.3, .alpha = 0.14};
}

BoundCoconeParametersDialog::BoundCoconeParametersDialog(
        int minimum_rho_exponent,
        int minimum_alpha_exponent,
        const BoundCoconeParameters& input,
        std::optional<BoundCoconeParameters>& parameters)
        : QDialog(parent_for_dialog()), m_parameters(parameters)
{
        ui.setupUi(this);
        setWindowTitle("BoundCocone Parameters");

        if (!(-10 <= minimum_rho_exponent && minimum_rho_exponent < 0))
        {
                error(reinterpret_cast<const char*>(u8"Error BoundCocone minimum ρ exponent: ")
                      + to_string(minimum_rho_exponent));
        }
        if (!(-10 <= minimum_alpha_exponent && minimum_alpha_exponent < 0))
        {
                error(reinterpret_cast<const char*>(u8"Error BoundCocone minimum α exponent: ")
                      + to_string(minimum_alpha_exponent));
        }

        double min_rho = std::pow(10, minimum_rho_exponent);
        double max_rho = 1 - min_rho;
        double min_alpha = std::pow(10, minimum_alpha_exponent);
        double max_alpha = 1 - min_alpha;

        if (!(min_rho <= input.rho && input.rho <= max_rho))
        {
                error(reinterpret_cast<const char*>(u8"BoundCocone parameter ρ range error: ρ = ")
                      + to_string(input.rho) + ", range = (" + to_string(min_rho) + ", " + to_string(max_rho) + ")");
        }
        if (!(min_alpha <= input.alpha && input.alpha <= max_alpha))
        {
                error(reinterpret_cast<const char*>(u8"BoundCocone parameter α range error: α = ")
                      + to_string(input.alpha) + ", range = (" + to_string(min_alpha) + ", " + to_string(max_alpha)
                      + ")");
        }

        m_min_rho = min_rho;
        m_max_rho = max_rho;
        m_min_alpha = min_alpha;
        m_max_alpha = max_alpha;

        ui.doubleSpinBox_rho->setDecimals(std::abs(minimum_rho_exponent));
        ui.doubleSpinBox_rho->setMinimum(min_rho);
        ui.doubleSpinBox_rho->setMaximum(max_rho);
        ui.doubleSpinBox_rho->setSingleStep(min_rho);
        ui.doubleSpinBox_rho->setValue(input.rho);

        ui.doubleSpinBox_alpha->setDecimals(std::abs(minimum_alpha_exponent));
        ui.doubleSpinBox_alpha->setMinimum(min_alpha);
        ui.doubleSpinBox_alpha->setMaximum(max_alpha);
        ui.doubleSpinBox_alpha->setSingleStep(min_alpha);
        ui.doubleSpinBox_alpha->setValue(input.alpha);
}

void BoundCoconeParametersDialog::done(int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        double rho = ui.doubleSpinBox_rho->value();
        if (!(m_min_rho <= rho && rho <= m_max_rho))
        {
                std::string msg = reinterpret_cast<const char*>(u8"ρ range error (") + to_string(m_min_rho) + ", "
                                  + to_string(m_max_rho) + ")";
                dialog::message_critical(msg);
                return;
        }

        double alpha = ui.doubleSpinBox_alpha->value();
        if (!(m_min_alpha <= alpha && alpha <= m_max_alpha))
        {
                std::string msg = reinterpret_cast<const char*>(u8"α range error (") + to_string(m_min_alpha) + ", "
                                  + to_string(m_max_alpha) + ")";
                dialog::message_critical(msg);
                return;
        }

        m_parameters.emplace();
        m_parameters->rho = rho;
        m_parameters->alpha = alpha;

        QDialog::done(r);
}

std::optional<BoundCoconeParameters> BoundCoconeParametersDialog::show()
{
        std::optional<BoundCoconeParameters> parameters;

        BoundCoconeParameters current;
        {
                std::lock_guard lg(g_mutex);
                current = g_parameters;
        }

        QtObjectInDynamicMemory w(
                new BoundCoconeParametersDialog(MINIMUM_RHO_EXPONENT, MINIMUM_ALPHA_EXPONENT, current, parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }
        {
                std::lock_guard lg(g_mutex);
                g_parameters = *parameters;
        }
        return parameters;
}

BoundCoconeParameters BoundCoconeParametersDialog::current()
{
        std::lock_guard lg(g_mutex);
        return g_parameters;
}
}
