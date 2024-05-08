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

#include <src/com/error.h>
#include <src/com/exponent.h>

#include <algorithm>
#include <optional>

namespace ns::filter::core::test::filters
{
template <typename T>
class VarianceCorrection final
{
        std::optional<T> last_time_;
        T last_k_;

        [[nodiscard]] static T correction(const T dt)
        {
                return std::min(T{30}, 1 + power<3>(dt) / 10'000);
        }

public:
        VarianceCorrection()
        {
                reset();
        }

        void reset()
        {
                last_time_.reset();
                last_k_ = 1;
        }

        [[nodiscard]] T update(const T time)
        {
                const T dt = last_time_ ? (time - *last_time_) : 1000;
                ASSERT(dt >= 0);
                const T k = (dt < 5) ? 1 : correction(dt);
                ASSERT(k >= 1);
                const T res = (last_k_ + k) / 2;
                last_time_ = time;
                last_k_ = res;
                return square(res);
        }
};
} // namespace ns::filter::core::test::filters
