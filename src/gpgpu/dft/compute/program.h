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
#include "gpgpu/com/groups.h"
#include "graphics/opengl/objects.h"

#include <complex>

template <typename T>
class DeviceProg final
{
        const int m_group_size_1d;
        opengl::ComputeProgram m_bit_reverse;
        opengl::ComputeProgram m_fft;
        opengl::ComputeProgram m_rows_mul_d;
        opengl::ComputeProgram m_copy_input;
        opengl::ComputeProgram m_copy_output;

public:
        DeviceProg(int group_size_1d, vec2i group_size_2d);

        void bit_reverse(int max_threads, int N_mask, int N_bits, DeviceMemory<std::complex<T>>* data) const
        {
                m_bit_reverse.set_uniform_unsigned(0, max_threads);
                m_bit_reverse.set_uniform_unsigned(1, N_mask);
                m_bit_reverse.set_uniform_unsigned(2, N_bits);
                data->bind(0);
                m_bit_reverse.dispatch_compute(group_count(max_threads, m_group_size_1d), 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        void fft(int max_threads, bool inverse, T Two_PI_Div_M, int N_2_mask, int N_2_bits, int M_2,
                 DeviceMemory<std::complex<T>>* data) const
        {
                m_fft.set_uniform(0, inverse);
                m_fft.set_uniform_unsigned(1, max_threads);
                m_fft.set_uniform_unsigned(2, N_2_mask);
                m_fft.set_uniform_unsigned(3, N_2_bits);
                m_fft.set_uniform_unsigned(4, M_2);
                m_fft.set_uniform(5, Two_PI_Div_M);
                data->bind(0);
                m_fft.dispatch_compute(group_count(max_threads, m_group_size_1d), 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        // Умножение на диагональ, формулы 13.20, 13.30.
        void rows_mul_d(vec2i groups, int columns, int rows, const DeviceMemory<std::complex<T>>& D,
                        DeviceMemory<std::complex<T>>* data) const
        {
                m_rows_mul_d.set_uniform(0, columns);
                m_rows_mul_d.set_uniform(1, rows);
                D.bind(0);
                data->bind(1);
                m_rows_mul_d.dispatch_compute(groups[0], groups[1], 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        void copy_input(vec2i groups, bool source_srgb, const GLuint64 tex, DeviceMemory<std::complex<T>>* data)
        {
                m_copy_input.set_uniform(0, source_srgb);
                m_copy_input.set_uniform_handle(1, tex);
                data->bind(0);
                m_copy_input.dispatch_compute(groups[0], groups[1], 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        void copy_output(vec2i groups, T to_mul, const GLuint64 tex, const DeviceMemory<std::complex<T>>& data)
        {
                m_copy_output.set_uniform(0, to_mul);
                m_copy_output.set_uniform_handle(1, tex);
                data.bind(0);
                m_copy_output.dispatch_compute(groups[0], groups[1], 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }
};

template <typename T>
class DeviceProgMul final
{
        opengl::ComputeProgram m_rows_to_buffer;
        opengl::ComputeProgram m_rows_from_buffer;
        opengl::ComputeProgram m_columns_to_buffer;
        opengl::ComputeProgram m_columns_from_buffer;

public:
        DeviceProgMul(vec2i group_size_2d);

        // Функции подстановки переменных, формулы 13.4, 13.27, 13.28, 13.32.

        void rows_to_buffer(vec2i groups, bool inverse, int M1, int N1, int N2, const DeviceMemory<std::complex<T>>& data,
                            DeviceMemory<std::complex<T>>* buffer) const
        {
                m_rows_to_buffer.set_uniform(0, inverse);
                m_rows_to_buffer.set_uniform(1, M1);
                //
                m_rows_to_buffer.set_uniform(3, N1);
                m_rows_to_buffer.set_uniform(4, N2);
                data.bind(0);
                buffer->bind(1);
                m_rows_to_buffer.dispatch_compute(groups[0], groups[1], 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        void rows_from_buffer(vec2i groups, bool inverse, int M1, int N1, int N2, DeviceMemory<std::complex<T>>* data,
                              const DeviceMemory<std::complex<T>>& buffer) const
        {
                m_rows_from_buffer.set_uniform(0, inverse);
                m_rows_from_buffer.set_uniform(1, M1);
                //
                m_rows_from_buffer.set_uniform(3, N1);
                m_rows_from_buffer.set_uniform(4, N2);
                data->bind(0);
                buffer.bind(1);
                m_rows_from_buffer.dispatch_compute(groups[0], groups[1], 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        void columns_to_buffer(vec2i groups, bool inverse, int M2, int N1, int N2, const DeviceMemory<std::complex<T>>& data,
                               DeviceMemory<std::complex<T>>* buffer) const
        {
                m_columns_to_buffer.set_uniform(0, inverse);
                //
                m_columns_to_buffer.set_uniform(2, M2);
                m_columns_to_buffer.set_uniform(3, N1);
                m_columns_to_buffer.set_uniform(4, N2);
                data.bind(0);
                buffer->bind(1);
                m_columns_to_buffer.dispatch_compute(groups[0], groups[1], 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        void columns_from_buffer(vec2i groups, bool inverse, int M2, int N1, int N2, DeviceMemory<std::complex<T>>* data,
                                 const DeviceMemory<std::complex<T>>& buffer) const
        {
                m_columns_from_buffer.set_uniform(0, inverse);
                //
                m_columns_from_buffer.set_uniform(2, M2);
                m_columns_from_buffer.set_uniform(3, N1);
                m_columns_from_buffer.set_uniform(4, N2);
                data->bind(0);
                buffer.bind(1);
                m_columns_from_buffer.dispatch_compute(groups[0], groups[1], 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }
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

        void exec(bool inverse, int data_size, DeviceMemory<std::complex<T>>* global_data) const
        {
                m_fft.set_uniform(0, inverse);
                m_fft.set_uniform_unsigned(1, data_size);
                global_data->bind(0);
                m_fft.dispatch_compute(group_count(data_size, m_shared_size), 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }
};
