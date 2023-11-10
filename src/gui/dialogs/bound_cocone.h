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

#pragma once

#include "ui_bound_cocone.h"

#include <optional>

namespace ns::gui::dialog
{
struct BoundCoconeParameters final
{
        double rho;
        double alpha;
};

class BoundCoconeParametersDialog final : public QDialog
{
        Q_OBJECT

private:
        Ui::BoundCoconeParametersDialog ui_;

        double min_rho_;
        double max_rho_;
        double min_alpha_;
        double max_alpha_;

        std::optional<BoundCoconeParameters>* const parameters_;

        BoundCoconeParametersDialog(
                int minimum_rho_exponent,
                int minimum_alpha_exponent,
                const BoundCoconeParameters& input,
                std::optional<BoundCoconeParameters>* parameters);

        void done(int r) override;

public:
        [[nodiscard]] static std::optional<BoundCoconeParameters> show();
        [[nodiscard]] static BoundCoconeParameters current();
};
}
