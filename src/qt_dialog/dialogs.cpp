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

#include "dialogs.h"

#include "bound_cocone_parameters.h"
#include "source_error.h"

void show_source_error(QWidget* parent, const std::string& msg, const std::string& source)
{
        SourceError dlg(parent);
        dlg.setWindowTitle("Source error");
        dlg.set_text(msg.c_str(), source.c_str());
        dlg.exec();
}

bool edit_bound_cocone_parameters(QWidget* parent, int digits, double* rho, double* alpha)
{
        BoundCoconeParameters dlg(parent);
        dlg.setWindowTitle("BOUND COCONE parameters");
        dlg.set_parameters(digits, *rho, *alpha);
        if (!dlg.exec())
        {
                return false;
        }
        dlg.get_parameters(rho, alpha);
        return true;
}
