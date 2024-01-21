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

#include "filter_0.h"

#include <src/filter/core/consistency.h>
#include <src/filter/filters/filter.h>
#include <src/filter/filters/measurement.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace ns::filter::filters::position
{
template <std::size_t N, typename T>
class Position0 final : public FilterPosition<N, T>
{
        T reset_dt_;
        T linear_dt_;
        std::optional<T> gate_;
        std::unique_ptr<Filter0<N, T>> filter_;

        core::NormalizedSquared<N, T> nees_position_;
        core::NormalizedSquared<1, T> nees_speed_;
        core::NormalizedSquared<1, T> nis_;

        std::optional<T> last_predict_time_;
        std::optional<T> last_update_time_;

        void add_nees_checks(const TrueData<N, T>& true_data);

        void check_time(T time) const;

public:
        Position0(T reset_dt, T linear_dt, std::optional<T> gate, T theta, T process_variance);

        [[nodiscard]] std::optional<UpdateInfo<N, T>> update(const Measurements<N, T>& m) override;
        [[nodiscard]] std::optional<UpdateInfo<N, T>> predict(const Measurements<N, T>& m) override;
        [[nodiscard]] std::string consistency_string() const override;

        [[nodiscard]] bool empty() const;
};
}
