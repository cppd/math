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

#include "device_mem.h"

#include "com/math.h"
#include "com/vec.h"
#include "graphics/objects.h"

#include <complex>

template <typename FP>
class DeviceProg final
{
        ComputeProgram m_reverse;
        ComputeProgram m_FFT;
        ComputeProgram m_rows_mul_to_buffer;
        ComputeProgram m_rows_mul_fr_buffer;
        ComputeProgram m_cols_mul_to_buffer;
        ComputeProgram m_cols_mul_fr_buffer;
        ComputeProgram m_rows_mul_D;
        ComputeProgram m_move_to_input;
        ComputeProgram m_move_to_output;

public:
        DeviceProg();

        void reverse(int blocks, int threads, int max_threads, int N_mask, int N_bits, DeviceMemory<std::complex<FP>>* data) const
        {
                m_reverse.set_uniform_unsigned(0, max_threads);
                m_reverse.set_uniform_unsigned(1, N_mask);
                m_reverse.set_uniform_unsigned(2, N_bits);
                data->bind(0);
                m_reverse.dispatch_compute(blocks, 1, 1, threads, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        void FFT(int blocks, int threads, bool inv, int max_threads, FP Two_PI_Div_M, int N_2_mask, int N_2_bits, int M_2,
                 DeviceMemory<std::complex<FP>>* data) const
        {
                m_FFT.set_uniform_unsigned(0, inv ? 1 : 0);
                m_FFT.set_uniform_unsigned(1, max_threads);
                m_FFT.set_uniform_unsigned(2, N_2_mask);
                m_FFT.set_uniform_unsigned(3, N_2_bits);
                m_FFT.set_uniform_unsigned(4, M_2);
                m_FFT.set_uniform(5, Two_PI_Div_M);
                data->bind(0);
                m_FFT.dispatch_compute(blocks, 1, 1, threads, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        // Функции подстановки переменных, формулы 13.4, 13.27, 13.28, 13.32.
        void rows_mul_to_buffer(vec2i blocks, vec2i threads, bool inv, int M1, int N1, int N2,
                                const DeviceMemory<std::complex<FP>>& data, DeviceMemory<std::complex<FP>>* buffer) const
        {
                m_rows_mul_to_buffer.set_uniform(0, inv ? 1 : 0);
                m_rows_mul_to_buffer.set_uniform(1, M1);
                m_rows_mul_to_buffer.set_uniform(2, N1);
                m_rows_mul_to_buffer.set_uniform(3, N2);
                data.bind(0);
                buffer->bind(1);
                m_rows_mul_to_buffer.dispatch_compute(blocks[0], blocks[1], 1, threads[0], threads[1], 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }
        void rows_mul_fr_buffer(vec2i blocks, vec2i threads, bool inv, int M1, int N1, int N2,
                                DeviceMemory<std::complex<FP>>* data, const DeviceMemory<std::complex<FP>>& buffer) const
        {
                m_rows_mul_fr_buffer.set_uniform(0, inv ? 1 : 0);
                m_rows_mul_fr_buffer.set_uniform(1, M1);
                m_rows_mul_fr_buffer.set_uniform(2, N1);
                m_rows_mul_fr_buffer.set_uniform(3, N2);
                data->bind(0);
                buffer.bind(1);
                m_rows_mul_fr_buffer.dispatch_compute(blocks[0], blocks[1], 1, threads[0], threads[1], 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }
        void cols_mul_to_buffer(vec2i blocks, vec2i threads, bool inv, int M2, int N1, int N2,
                                const DeviceMemory<std::complex<FP>>& data, DeviceMemory<std::complex<FP>>* buffer) const
        {
                m_cols_mul_to_buffer.set_uniform(0, inv ? 1 : 0);
                m_cols_mul_to_buffer.set_uniform(1, M2);
                m_cols_mul_to_buffer.set_uniform(2, N1);
                m_cols_mul_to_buffer.set_uniform(3, N2);
                data.bind(0);
                buffer->bind(1);
                m_cols_mul_to_buffer.dispatch_compute(blocks[0], blocks[1], 1, threads[0], threads[1], 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }
        void cols_mul_fr_buffer(vec2i blocks, vec2i threads, bool inv, int M2, int N1, int N2,
                                DeviceMemory<std::complex<FP>>* data, const DeviceMemory<std::complex<FP>>& buffer) const
        {
                m_cols_mul_fr_buffer.set_uniform(0, inv ? 1 : 0);
                m_cols_mul_fr_buffer.set_uniform(1, M2);
                m_cols_mul_fr_buffer.set_uniform(2, N1);
                m_cols_mul_fr_buffer.set_uniform(3, N2);
                data->bind(0);
                buffer.bind(1);
                m_cols_mul_fr_buffer.dispatch_compute(blocks[0], blocks[1], 1, threads[0], threads[1], 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        // Умножение на диагональ, формулы 13.20, 13.30.
        void rows_mul_D(vec2i blocks, vec2i threads, int columns, int rows, const DeviceMemory<std::complex<FP>>& D,
                        DeviceMemory<std::complex<FP>>* data) const
        {
                m_rows_mul_D.set_uniform(0, columns);
                m_rows_mul_D.set_uniform(1, rows);
                D.bind(0);
                data->bind(1);
                m_rows_mul_D.dispatch_compute(blocks[0], blocks[1], 1, threads[0], threads[1], 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }

        void move_to_input(vec2i blocks, vec2i threads, int width, int height, bool source_srgb, const GLuint64 tex,
                           DeviceMemory<std::complex<FP>>* data)
        {
                m_move_to_input.set_uniform(0, width);
                m_move_to_input.set_uniform(1, height);
                m_move_to_input.set_uniform(2, source_srgb ? 1 : 0);
                m_move_to_input.set_uniform_handle(3, tex);
                data->bind(0);
                m_move_to_input.dispatch_compute(blocks[0], blocks[1], 1, threads[0], threads[1], 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }
        void move_to_output(vec2i blocks, vec2i threads, int width, int height, FP to_mul, const GLuint64 tex,
                            const DeviceMemory<std::complex<FP>>& data)
        {
                m_move_to_output.set_uniform(0, width);
                m_move_to_output.set_uniform(1, height);
                m_move_to_output.set_uniform(2, to_mul);
                m_move_to_output.set_uniform_handle(3, tex);
                data.bind(0);
                m_move_to_output.dispatch_compute(blocks[0], blocks[1], 1, threads[0], threads[1], 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }
};

template <typename FP>
class DeviceProgFFTRadix2 final
{
        const int m_group_size, m_shared_size;
        ComputeProgram m_FFT;

public:
        DeviceProgFFTRadix2(int N, int shared_size, bool reverse_input, int group_size);

        void exec(bool inv, int data_size, DeviceMemory<std::complex<FP>>* global_data) const
        {
                m_FFT.set_uniform_unsigned(0, inv ? 1 : 0);
                m_FFT.set_uniform_unsigned(1, data_size);
                global_data->bind(0);
                m_FFT.dispatch_compute(get_group_count(data_size, m_shared_size), 1, 1, m_group_size, 1, 1);
                glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }
};

extern template class DeviceProg<float>;
extern template class DeviceProg<double>;
extern template class DeviceProgFFTRadix2<float>;
extern template class DeviceProgFFTRadix2<double>;
