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

#include "testing.h"

#include <src/gui/dialogs/message.h>

namespace process
{
std::function<void(ProgressRatioList*)> action_self_test(test::SelfTestType test_type, bool with_confirmation)
{
        if (with_confirmation)
        {
                std::optional<bool> yes = gui::dialog::message_question_default_yes("Run the Self-Test?");
                if (!yes || !*yes)
                {
                        return nullptr;
                }
        }

        return [=](ProgressRatioList* progress_list)
        {
                test::self_test(test_type, progress_list);
        };
}
}
