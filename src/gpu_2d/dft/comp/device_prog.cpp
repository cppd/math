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

#include "device_prog.h"

#include "com/bits.h"
#include "com/math.h"

// clang-format off
constexpr const char dft_fft_shader[]
{
#include "dft_fft.comp.str"
};
constexpr const char dft_copy_shader[]
{
#include "dft_copy.comp.str"
};
constexpr const char dft_mul_shader[]
{
#include "dft_mul.comp.str"
};
// clang-format on

namespace
{
template <typename FP>
std::string data_types();

template <>
std::string data_types<float>()
{
        std::string s;
        s += "#define complex vec2\n";
        s += "#define float_point float\n";
        s += "const float PI = " + std::string(PI_STR) + ";\n";
        s += "\n";
        return s;
}

template <>
std::string data_types<double>()
{
        std::string s;
        s += "#define complex dvec2\n";
        s += "#define float_point double\n";
        s += "const double PI = " + std::string(PI_STR) + "LF;\n";
        s += "\n";
        return s;
}

std::string reverse_shader_source()
{
        std::string s;
        s += "#define function_reverse\n\n";
        return s + dft_fft_shader;
}

std::string fft_shader_source()
{
        std::string s;
        s += "#define function_FFT\n\n";
        return s + dft_fft_shader;
}

std::string rows_mul_to_buffer_shader_source()
{
        std::string s;
        s += "#define function_rows_mul_to_buffer\n\n";
        return s + dft_mul_shader;
}

std::string rows_mul_fr_buffer_shader_source()
{
        std::string s;
        s += "#define function_rows_mul_fr_buffer\n\n";
        return s + dft_mul_shader;
}

std::string cols_mul_to_buffer_shader_source()
{
        std::string s;
        s += "#define function_cols_mul_to_buffer\n\n";
        return s + dft_mul_shader;
}

std::string cols_mul_fr_buffer_shader_source()
{
        std::string s;
        s += "#define function_cols_mul_fr_buffer\n\n";
        return s + dft_mul_shader;
}

std::string rows_mul_d_shader_source()
{
        std::string s;
        s += "#define function_rows_mul_D\n\n";
        return s + dft_mul_shader;
}

std::string move_to_input_shader_source()
{
        std::string s;
        s += "#define function_move_to_input\n\n";
        return s + dft_copy_shader;
}

std::string move_to_output_shader_source()
{
        std::string s;
        s += "#define function_move_to_output\n\n";
        return s + dft_copy_shader;
}

std::string fft_radix_2_shader_source(int N, int shared_size, bool reverse_input)
{
        std::string s;
        s += "#define function_FFT_radix_2\n\n";
        s += "const uint N = " + std::to_string(N) + ";\n";
        s += "const uint N_MASK = " + std::to_string(N - 1) + ";\n";
        s += "const uint N_BITS = " + std::to_string(binary_size(N)) + ";\n";
        s += "const uint SHARED_SIZE = " + std::to_string(shared_size) + ";\n";
        s += "const bool REVERSE_INPUT = " + (reverse_input ? std::string("true") : std::string("false")) + ";\n";
        return s + dft_fft_shader;
}
}

template <typename FP>
DeviceProg<FP>::DeviceProg()
        : m_reverse(opengl::ComputeShader(data_types<FP>() + reverse_shader_source())),
          m_FFT(opengl::ComputeShader(data_types<FP>() + fft_shader_source())),
          m_rows_mul_to_buffer(opengl::ComputeShader(data_types<FP>() + rows_mul_to_buffer_shader_source())),
          m_rows_mul_fr_buffer(opengl::ComputeShader(data_types<FP>() + rows_mul_fr_buffer_shader_source())),
          m_cols_mul_to_buffer(opengl::ComputeShader(data_types<FP>() + cols_mul_to_buffer_shader_source())),
          m_cols_mul_fr_buffer(opengl::ComputeShader(data_types<FP>() + cols_mul_fr_buffer_shader_source())),
          m_rows_mul_D(opengl::ComputeShader(data_types<FP>() + rows_mul_d_shader_source())),
          m_move_to_input(opengl::ComputeShader(data_types<FP>() + move_to_input_shader_source())),
          m_move_to_output(opengl::ComputeShader(data_types<FP>() + move_to_output_shader_source()))
{
}

template <typename FP>
DeviceProgFFTRadix2<FP>::DeviceProgFFTRadix2(int N, int shared_size, bool reverse_input, int group_size)
        : m_group_size(group_size),
          m_shared_size(shared_size),
          m_FFT(opengl::ComputeShader(data_types<FP>() + fft_radix_2_shader_source(N, shared_size, reverse_input)))
{
}

template class DeviceProg<float>;
template class DeviceProg<double>;
template class DeviceProgFFTRadix2<float>;
template class DeviceProgFFTRadix2<double>;
