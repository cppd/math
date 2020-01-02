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

#pragma once

#include <iterator>
#include <type_traits>

template <typename T>
class Span
{
        T* const m_pointer;
        const size_t m_size;

public:
        using value_type = std::remove_cv_t<T>;

        constexpr Span(T* pointer, size_t size) : m_pointer(pointer), m_size(size)
        {
        }

        template <class Container>
        constexpr Span(Container& c) : m_pointer(std::data(c)), m_size(std::size(c))
        {
        }

        template <class Container>
        constexpr Span(const Container& c) : m_pointer(std::data(c)), m_size(std::size(c))
        {
        }

        constexpr T& operator[](size_t i) const
        {
                return m_pointer[i];
        }

        constexpr T* data() const
        {
                return m_pointer;
        }

        constexpr size_t size() const
        {
                return m_size;
        }

        constexpr bool empty() const
        {
                return m_size == 0;
        }
};
