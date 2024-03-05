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

#include "consistency.h"
#include "filter_2.h"
#include "init.h"

#include <src/filter/filters/filter.h>
#include <src/filter/filters/measurement.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace ns::filter::filters::position
{
template <std::size_t N, typename T>
class Position2 final : public FilterPosition<N, T>
{
        T reset_dt_;
        T linear_dt_;
        std::optional<T> gate_;
        std::unique_ptr<Filter2<N, T>> filter_;
        Init<T> init_;

        Nees<N, T> nees_;
        Nis<N, T> nis_;

        std::optional<T> last_predict_time_;
        std::optional<T> last_update_time_;

        void check_time(T time) const;

public:
        Position2(T reset_dt, T linear_dt, std::optional<T> gate, T theta, T process_variance, const Init<T>& init);

        [[nodiscard]] std::optional<UpdateInfo<N, T>> update(const Measurements<N, T>& m) override;
        [[nodiscard]] std::optional<UpdateInfo<N, T>> predict(const Measurements<N, T>& m) override;
        [[nodiscard]] std::string consistency_string() const override;

        [[nodiscard]] bool empty() const;

        [[nodiscard]] numerical::Vector<N, T> position() const;
        [[nodiscard]] numerical::Matrix<N, N, T> position_p() const;
        [[nodiscard]] numerical::Vector<N, T> velocity() const;
        [[nodiscard]] numerical::Matrix<N, N, T> velocity_p() const;
        [[nodiscard]] numerical::Vector<2 * N, T> position_velocity() const;
        [[nodiscard]] numerical::Matrix<2 * N, 2 * N, T> position_velocity_p() const;
        [[nodiscard]] numerical::Vector<3 * N, T> position_velocity_acceleration() const;
        [[nodiscard]] numerical::Matrix<3 * N, 3 * N, T> position_velocity_acceleration_p() const;
};
}
