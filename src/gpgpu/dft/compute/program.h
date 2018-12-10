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

#include "memory.h"

#include "com/vec.h"
#include "graphics/opengl/objects.h"

#include <complex>

template <typename T>
class DeviceProg final
{
        const int m_group_size;
        opengl::ComputeProgram m_bit_reverse;
        opengl::ComputeProgram m_fft;

public:
        DeviceProg(int group_size);

        void bit_reverse(int max_threads, int N_mask, int N_bits, DeviceMemory<std::complex<T>>* data) const;

        void fft(int max_threads, bool inverse, T Two_PI_Div_M, int N_2_mask, int N_2_bits, int M_2,
                 DeviceMemory<std::complex<T>>* data) const;
};

template <typename T>
class DeviceProgCopy final
{
        const vec2i m_group_count;
        opengl::ComputeProgram m_copy_input;
        opengl::ComputeProgram m_copy_output;

public:
        DeviceProgCopy(vec2i group_size, int n1, int n2);

        void copy_input(bool source_srgb, const GLuint64 tex, DeviceMemory<std::complex<T>>* data);
        void copy_output(T to_mul, const GLuint64 tex, const DeviceMemory<std::complex<T>>& data);
};

template <typename T>
class DeviceProgMul final
{
        const vec2i m_rows_to;
        const vec2i m_rows_from;
        const vec2i m_columns_to;
        const vec2i m_columns_from;
        opengl::ComputeProgram m_rows_to_buffer;
        opengl::ComputeProgram m_rows_from_buffer;
        opengl::ComputeProgram m_columns_to_buffer;
        opengl::ComputeProgram m_columns_from_buffer;

public:
        DeviceProgMul(vec2i group_size, int n1, int n2, int m1, int m2);

        // Функции подстановки переменных, формулы 13.4, 13.27, 13.28, 13.32.
        void rows_to_buffer(bool inverse, const DeviceMemory<std::complex<T>>& data, DeviceMemory<std::complex<T>>* buffer) const;
        void rows_from_buffer(bool inverse, DeviceMemory<std::complex<T>>* data,
                              const DeviceMemory<std::complex<T>>& buffer) const;
        void columns_to_buffer(bool inverse, const DeviceMemory<std::complex<T>>& data,
                               DeviceMemory<std::complex<T>>* buffer) const;
        void columns_from_buffer(bool inverse, DeviceMemory<std::complex<T>>* data,
                                 const DeviceMemory<std::complex<T>>& buffer) const;
};

template <typename T>
class DeviceProgMulD final
{
        const int m_n1;
        const int m_n2;
        const int m_m1;
        const int m_m2;
        const vec2i m_rows_d;
        const vec2i m_cols_d;
        opengl::ComputeProgram m_mul_d;

public:
        DeviceProgMulD(vec2i group_size, int n1, int n2, int m1, int m2);

        // Умножение на диагональ, формулы 13.20, 13.30.
        void rows_mul_d(const DeviceMemory<std::complex<T>>& d, DeviceMemory<std::complex<T>>* data) const;
        void columns_mul_d(const DeviceMemory<std::complex<T>>& d, DeviceMemory<std::complex<T>>* data) const;
};

template <typename T>
class DeviceProgFFTShared final
{
        const int m_n, m_n_bits, m_shared_size;
        opengl::ComputeProgram m_fft;

public:
        DeviceProgFFTShared(int n, int shared_size, int group_size, bool reverse_input);

        int n() const
        {
                return m_n;
        }

        int n_bits() const
        {
                return m_n_bits;
        }

        int shared_size() const
        {
                return m_shared_size;
        }

        void exec(bool inverse, int data_size, DeviceMemory<std::complex<T>>* global_data) const;
};
