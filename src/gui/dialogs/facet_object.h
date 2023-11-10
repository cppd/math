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

#include "ui_facet_object.h"

#include <optional>
#include <string>

namespace ns::gui::dialog
{
struct FacetObjectParameters final
{
        int facet_count;
};

class FacetObjectParametersDialog final : public QDialog
{
        Q_OBJECT

private:
        Ui::FacetObjectParametersDialog ui_;

        const int min_facet_count_;
        const int max_facet_count_;

        std::optional<FacetObjectParameters>* const parameters_;

        FacetObjectParametersDialog(
                int dimension,
                const std::string& object_name,
                int default_facet_count,
                int min_facet_count,
                int max_facet_count,
                std::optional<FacetObjectParameters>* parameters);

        void done(int r) override;

public:
        [[nodiscard]] static std::optional<FacetObjectParameters> show(
                int dimension,
                const std::string& object_name,
                int default_facet_count,
                int min_facet_count,
                int max_facet_count);
};
}
