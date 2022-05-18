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

#pragma once

#include <string>

namespace ns
{
class ProgressRatioControl
{
protected:
        ~ProgressRatioControl() = default;

public:
        struct Info final
        {
                unsigned value;
                unsigned maximum;
        };

        virtual void terminate_quietly() = 0;
        virtual void terminate_with_message() = 0;
        [[nodiscard]] virtual Info info() const = 0;
        [[nodiscard]] virtual std::string text() const = 0;
};

class ProgressRatios
{
protected:
        ~ProgressRatios() = default;

public:
        virtual void add_progress_ratio(ProgressRatioControl* ratio) = 0;
        virtual void delete_progress_ratio(const ProgressRatioControl* ratio) noexcept = 0;
};
}
