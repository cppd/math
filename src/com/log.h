/*
Copyright (C) 2017, 2018 Topological Manifold

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
#include <vector>

void log_init();
void log_exit() noexcept;

class ILogCallback
{
protected:
        ~ILogCallback() = default;

public:
        virtual void log(const std::string& msg) const noexcept = 0;
};

void set_log_callback(ILogCallback* callback) noexcept;

std::vector<std::string> format_log_message(const std::string& msg) noexcept;
void write_formatted_log_messages_to_stderr(const std::vector<std::string>& message);

void LOG(const std::string& msg) noexcept;
