/*
Copyright (C) 2017-2023 Topological Manifold

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
                const std::lock_guard lg(mutex_);
                return parameters_;
        }

        void write(const BoundCoconeParameters& parameters)
        {
                const std::lock_guard lg(mutex_);
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
        std::optional<BoundCoconeParameters>* const parameters)
        : QDialog(parent_for_dialog()),
          parameters_(parameters)
{
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

        ui_.setupUi(this);
        setWindowTitle("BoundCocone");

        min_rho_ = std::pow(10, minimum_rho_exponent);
        max_rho_ = 1 - min_rho_;
        min_alpha_ = std::pow(10, minimum_alpha_exponent);
        max_alpha_ = 1 - min_alpha_;

        ui_.double_spin_box_rho->setDecimals(std::abs(minimum_rho_exponent));
        ui_.double_spin_box_rho->setMinimum(min_rho_);
        ui_.double_spin_box_rho->setMaximum(max_rho_);
        ui_.double_spin_box_rho->setSingleStep(min_rho_);

        ui_.double_spin_box_alpha->setDecimals(std::abs(minimum_alpha_exponent));
        ui_.double_spin_box_alpha->setMinimum(min_alpha_);
        ui_.double_spin_box_alpha->setMaximum(max_alpha_);
        ui_.double_spin_box_alpha->setSingleStep(min_alpha_);

        //

        ui_.double_spin_box_rho->setValue(min_rho_);
        ui_.double_spin_box_alpha->setValue(min_alpha_);

        set_dialog_size(this);

        ui_.double_spin_box_rho->setValue(
                std::isfinite(input.rho) ? std::clamp(input.rho, min_rho_, max_rho_) : min_rho_);
        ui_.double_spin_box_alpha->setValue(
                std::isfinite(input.alpha) ? std::clamp(input.alpha, min_alpha_, max_alpha_) : min_alpha_);
}

void BoundCoconeParametersDialog::done(const int r)
{
        if (r != QDialog::Accepted)
        {
                QDialog::done(r);
                return;
        }

        const double rho = ui_.double_spin_box_rho->value();
        if (!(min_rho_ <= rho && rho <= max_rho_))
        {
                const std::string msg =
                        reinterpret_cast<const char*>(u8"ρ must be in the range [") + to_string(min_rho_) + ", "
                        + to_string(max_rho_) + "]";
                dialog::message_critical(msg);
                return;
        }

        const double alpha = ui_.double_spin_box_alpha->value();
        if (!(min_alpha_ <= alpha && alpha <= max_alpha_))
        {
                const std::string msg =
                        reinterpret_cast<const char*>(u8"α must be in the range [") + to_string(min_alpha_) + ", "
                        + to_string(max_alpha_) + "]";
                dialog::message_critical(msg);
                return;
        }

        auto& parameters = parameters_->emplace();
        parameters.rho = rho;
        parameters.alpha = alpha;

        QDialog::done(r);
}

std::optional<BoundCoconeParameters> BoundCoconeParametersDialog::show()
{
        std::optional<BoundCoconeParameters> parameters;

        const QtObjectInDynamicMemory w(new BoundCoconeParametersDialog(
                MINIMUM_RHO_EXPONENT, MINIMUM_ALPHA_EXPONENT, dialog_parameters().read(), &parameters));

        if (!w->exec() || w.isNull())
        {
                return std::nullopt;
        }

        ASSERT(parameters);
        dialog_parameters().write(*parameters);

        return parameters;
}

BoundCoconeParameters BoundCoconeParametersDialog::current()
{
        return dialog_parameters().read();
}
}
