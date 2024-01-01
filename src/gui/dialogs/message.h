/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <optional>
#include <string>

namespace ns::gui::dialog
{
void message_critical(const std::string& message, bool with_parent = true);
void message_information(const std::string& message);
void message_warning(const std::string& message);
[[nodiscard]] std::optional<bool> message_question_default_yes(const std::string& message);
[[nodiscard]] std::optional<bool> message_question_default_no(const std::string& message);
}
