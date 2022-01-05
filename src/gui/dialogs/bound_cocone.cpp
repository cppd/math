/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <mutex>

namespace ns::gui::dialog
{
namespace
{
constexpr int MINIMUM_RHO_EXPONENT = -3;
constexpr int MINIMUM_ALPHA_EXPONENT = -3;

class DialogParameters final
{
        mutable std::mutex mutex_;
        BoundCoconeParameters parameters_{.rho = 0.3, .alpha = 0.14};

public:
        BoundCoconeParameters read() const
        {
                std::lock_guard lg(mutex_);
                return parameters_;
        }

        void write(const BoundCoconeParameters& parameters)
        {
                std::lock_guard lg(mutex_);
                parameters_ = parameters;
        }
};

DialogParameters& dialog_parameters()
{
        static DialogParameters parameters;
        return parameters;
}
}

BoundCoconeParametersDialog::BoundCoconeParametersDialog(
        const int minimum_rho_exponent,
        const int minimum_alpha_exponent,
        const BoundCoconeParameters& input,
        std::optional<BoundCoconeParameters>& parameters)
        : QDialog(parent_for_dialog()), parameters_(parameters)
{
        ui_.setupUi(this);
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

        min_rho_ = std::pow(10, minimum_rho_exponent);
        max_rho_ = 1 - min_rho_;
        min_alpha_ = std::pow(10, minimum_alpha_exponent);
        max_alpha_ = 1 - min_alpha_;

        const double rho = std::isfinite(input.rho) ? std::clamp(input.rho, min_rho_, max_rho_) : min_rho_;
        ui_.doubleSpinBox_rho->setDecimals(std::abs(minimum_rho_exponent));
        ui_.doubleSpinBox_rho->setMinimum(min_rho_);
        ui_.doubleSpinBox_rho->setMaximum(max_rho_);
        ui_.doubleSpinBox_rho->setSingleStep(min_rho_);
        ui_.doubleSpinBox_rho->setValue(rho);

        const double alpha = std::isfinite(input.alpha) ? std::clamp(input.alpha, min_alpha_, max_alpha_) : min_alpha_;
        ui_.doubleSpinBox_alpha->setDecimals(std::abs(minimum_alpha_exponent));
        ui_.doubleSpinBox_alpha->setMinimum(min_alpha_);
        ui_.doubleSpinBox_alpha->setMaximum(max_alpha_);
        ui_.doubleSpinBox_alpha->setSingleStep(min_alpha_);
        ui_.doubleSpinBox_alpha->setValue(alpha);
}

void BoundCoconeParametersDialog::done(const int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        const double rho = ui_.doubleSpinBox_rho->value();
        if (!(min_rho_ <= rho && rho <= max_rho_))
        {
                std::string msg = reinterpret_cast<const char*>(u8"ρ must be in the range [") + to_string(min_rho_)
                                  + ", " + to_string(max_rho_) + "]";
                dialog::message_critical(msg);
                return;
        }

        const double alpha = ui_.doubleSpinBox_alpha->value();
        if (!(min_alpha_ <= alpha && alpha <= max_alpha_))
        {
                std::string msg = reinterpret_cast<const char*>(u8"α must be in the range [") + to_string(min_alpha_)
                                  + ", " + to_string(max_alpha_) + "]";
                dialog::message_critical(msg);
                return;
        }

        parameters_.emplace();
        parameters_->rho = rho;
        parameters_->alpha = alpha;

        QDialog::done(r);
}

std::optional<BoundCoconeParameters> BoundCoconeParametersDialog::show()
{
        std::optional<BoundCoconeParameters> parameters;

        QtObjectInDynamicMemory w(new BoundCoconeParametersDialog(
                MINIMUM_RHO_EXPONENT, MINIMUM_ALPHA_EXPONENT, dialog_parameters().read(), parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }

        dialog_parameters().write(*parameters);

        return parameters;
}

BoundCoconeParameters BoundCoconeParametersDialog::current()
{
        return dialog_parameters().read();
}
}
