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
#include <src/com/math.h>
#include <src/com/print.h>

#include <mutex>

namespace ns::gui::dialog
{
namespace
{
constexpr int MINIMUM_RHO_EXPONENT = -3;
constexpr int MINIMUM_ALPHA_EXPONENT = -3;

std::mutex g_mutex;
BoundCoconeParameters g_parameters{.rho = 0.3, .alpha = 0.14};

BoundCoconeParameters read_current()
{
        std::lock_guard lg(g_mutex);
        return g_parameters;
}

void write_current(const BoundCoconeParameters& parameters)
{
        std::lock_guard lg(g_mutex);
        g_parameters = parameters;
}
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
                error(reinterpret_cast<const char*>(u8"BoundCocone minimum ρ exponent ")
                      + to_string(minimum_rho_exponent) + " must be in the range [-10, 0]");
        }
        if (!(-10 <= minimum_alpha_exponent && minimum_alpha_exponent < 0))
        {
                error(reinterpret_cast<const char*>(u8"BoundCocone minimum α exponent ")
                      + to_string(minimum_alpha_exponent) + " must be in the range [-10, 0]");
        }

        m_min_rho = std::pow(10, minimum_rho_exponent);
        m_max_rho = 1 - m_min_rho;
        m_min_alpha = std::pow(10, minimum_alpha_exponent);
        m_max_alpha = 1 - m_min_alpha;

        double rho = is_finite(input.rho) ? std::clamp(input.rho, m_min_rho, m_max_rho) : m_min_rho;
        ui.doubleSpinBox_rho->setDecimals(std::abs(minimum_rho_exponent));
        ui.doubleSpinBox_rho->setMinimum(m_min_rho);
        ui.doubleSpinBox_rho->setMaximum(m_max_rho);
        ui.doubleSpinBox_rho->setSingleStep(m_min_rho);
        ui.doubleSpinBox_rho->setValue(rho);

        double alpha = is_finite(input.alpha) ? std::clamp(input.alpha, m_min_alpha, m_max_alpha) : m_min_alpha;
        ui.doubleSpinBox_alpha->setDecimals(std::abs(minimum_alpha_exponent));
        ui.doubleSpinBox_alpha->setMinimum(m_min_alpha);
        ui.doubleSpinBox_alpha->setMaximum(m_max_alpha);
        ui.doubleSpinBox_alpha->setSingleStep(m_min_alpha);
        ui.doubleSpinBox_alpha->setValue(alpha);
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
                std::string msg = reinterpret_cast<const char*>(u8"ρ must be in the range [") + to_string(m_min_rho)
                                  + ", " + to_string(m_max_rho) + "]";
                dialog::message_critical(msg);
                return;
        }

        double alpha = ui.doubleSpinBox_alpha->value();
        if (!(m_min_alpha <= alpha && alpha <= m_max_alpha))
        {
                std::string msg = reinterpret_cast<const char*>(u8"α must be in the range [") + to_string(m_min_alpha)
                                  + ", " + to_string(m_max_alpha) + "]";
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

        QtObjectInDynamicMemory w(new BoundCoconeParametersDialog(
                MINIMUM_RHO_EXPONENT, MINIMUM_ALPHA_EXPONENT, read_current(), parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }

        write_current(*parameters);

        return parameters;
}

BoundCoconeParameters BoundCoconeParametersDialog::current()
{
        return read_current();
}
}
