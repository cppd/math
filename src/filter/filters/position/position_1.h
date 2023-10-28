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

#include "filter_1.h"
#include "init.h"

#include "../../consistency.h"
#include "../filter.h"
#include "../measurement.h"

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <memory>
#include <optional>
#include <string>

namespace ns::filter::filters::position
{
template <std::size_t N, typename T>
class Position1 final : public FilterPosition<N, T>
{
        T reset_dt_;
        T linear_dt_;
        std::optional<T> gate_;
        std::unique_ptr<Filter1<N, T>> filter_;
        Init<T> init_;

        NormalizedSquared<N, T> nees_position_;
        NormalizedSquared<1, T> nees_speed_;
        NormalizedSquared<1, T> nis_;

        std::optional<T> last_predict_time_;
        std::optional<T> last_update_time_;

        void add_nees_checks(const TrueData<N, T>& true_data);

        void check_time(T time) const;

public:
        Position1(T reset_dt, T linear_dt, std::optional<T> gate, T theta, T process_variance, const Init<T>& init);

        [[nodiscard]] std::optional<UpdateInfo<N, T>> update(const Measurements<N, T>& m) override;
        [[nodiscard]] std::optional<UpdateInfo<N, T>> predict(const Measurements<N, T>& m) override;
        [[nodiscard]] std::string consistency_string() const override;

        [[nodiscard]] bool empty() const;
        [[nodiscard]] Vector<N, T> velocity() const;
        [[nodiscard]] Matrix<N, N, T> velocity_p() const;
};
}
