/*
Copyright (C) 2017 Topological Manifold

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

#include "device_prog.h"

#include "com/bits.h"
#include "com/math.h"

// clang-format off
constexpr const char dft_fft[]
{
#include "dft_fft.comp.str"
};
constexpr const char dft_copy[]
{
#include "dft_copy.comp.str"
};
constexpr const char dft_mul[]
{
#include "dft_mul.comp.str"
};
// clang-format on

namespace
{
template <typename FP>
std::string get_data_types();

template <>
std::string get_data_types<float>()
{
        std::string s;
        s += "#define complex vec2\n";
        s += "#define float_point float\n";
        s += "const float PI = " + std::string(PI_STR) + ";\n";
        s += "\n";
        return s;
}

template <>
std::string get_data_types<double>()
{
        std::string s;
        s += "#define complex dvec2\n";
        s += "#define float_point double\n";
        s += "const double PI = " + std::string(PI_STR) + "LF;\n";
        s += "\n";
        return s;
}

std::string get_reverse()
{
        std::string s;
        s += "#define function_reverse\n\n";
        return s + dft_fft;
}

std::string get_FFT()
{
        std::string s;
        s += "#define function_FFT\n\n";
        return s + dft_fft;
}

std::string get_rows_mul_to_buffer()
{
        std::string s;
        s += "#define function_rows_mul_to_buffer\n\n";
        return s + dft_mul;
}

std::string get_rows_mul_fr_buffer()
{
        std::string s;
        s += "#define function_rows_mul_fr_buffer\n\n";
        return s + dft_mul;
}

std::string get_cols_mul_to_buffer()
{
        std::string s;
        s += "#define function_cols_mul_to_buffer\n\n";
        return s + dft_mul;
}

std::string get_cols_mul_fr_buffer()
{
        std::string s;
        s += "#define function_cols_mul_fr_buffer\n\n";
        return s + dft_mul;
}

std::string get_rows_mul_D()
{
        std::string s;
        s += "#define function_rows_mul_D\n\n";
        return s + dft_mul;
}

std::string get_move_to_input()
{
        std::string s;
        s += "#define function_move_to_input\n\n";
        return s + dft_copy;
}

std::string get_move_to_output()
{
        std::string s;
        s += "#define function_move_to_output\n\n";
        return s + dft_copy;
}

std::string get_FFT_radix_2(int N, int shared_size, bool reverse_input)
{
        std::string s;
        s += "#define function_FFT_radix_2\n\n";
        s += "const uint N = " + std::to_string(N) + ";\n";
        s += "const uint N_MASK = " + std::to_string(N - 1) + ";\n";
        s += "const uint N_BITS = " + std::to_string(get_bin_size(N)) + ";\n";
        s += "const uint SHARED_SIZE = " + std::to_string(shared_size) + ";\n";
        s += "const bool REVERSE_INPUT = " + (reverse_input ? std::string("true") : std::string("false")) + ";\n";
        return s + dft_fft;
}
}

template <typename FP>
DeviceProg<FP>::DeviceProg()
        : m_reverse(ComputeShader(get_data_types<FP>() + get_reverse())),
          m_FFT(ComputeShader(get_data_types<FP>() + get_FFT())),
          m_rows_mul_to_buffer(ComputeShader(get_data_types<FP>() + get_rows_mul_to_buffer())),
          m_rows_mul_fr_buffer(ComputeShader(get_data_types<FP>() + get_rows_mul_fr_buffer())),
          m_cols_mul_to_buffer(ComputeShader(get_data_types<FP>() + get_cols_mul_to_buffer())),
          m_cols_mul_fr_buffer(ComputeShader(get_data_types<FP>() + get_cols_mul_fr_buffer())),
          m_rows_mul_D(ComputeShader(get_data_types<FP>() + get_rows_mul_D())),
          m_move_to_input(ComputeShader(get_data_types<FP>() + get_move_to_input())),
          m_move_to_output(ComputeShader(get_data_types<FP>() + get_move_to_output()))
{
}

template <typename FP>
DeviceProgFFTRadix2<FP>::DeviceProgFFTRadix2(int N, int shared_size, bool reverse_input, int group_size)
        : m_group_size(group_size),
          m_shared_size(shared_size),
          m_FFT(ComputeShader(get_data_types<FP>() + get_FFT_radix_2(N, shared_size, reverse_input)))
{
}

template class DeviceProg<float>;
template class DeviceProg<double>;
template class DeviceProgFFTRadix2<float>;
template class DeviceProgFFTRadix2<double>;
