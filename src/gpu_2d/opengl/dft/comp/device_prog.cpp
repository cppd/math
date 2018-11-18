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

// clang-format off
constexpr const char dft_fft_global_shader[]
{
#include "dft_fft_global.comp.str"
};
constexpr const char dft_fft_shared_shader[]
{
#include "dft_fft_shared.comp.str"
};
constexpr const char dft_bit_reverse_shader[]
{
#include "dft_bit_reverse.comp.str"
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
template <typename T>
std::string floating_point_source();

template <>
std::string floating_point_source<float>()
{
        std::string s;
        s += "#define complex vec2\n";
        s += "#define float_point float\n";
        s += "const float PI = " + std::string(PI_STR) + ";\n";
        s += "\n";
        return s;
}

template <>
std::string floating_point_source<double>()
{
        std::string s;
        s += "#define complex dvec2\n";
        s += "#define float_point double\n";
        s += "const double PI = " + std::string(PI_STR) + "LF;\n";
        s += "\n";
        return s;
}

template <typename T>
std::string bit_reverse_source()
{
        std::string s;
        s += floating_point_source<T>();
        return s + dft_bit_reverse_shader;
}

template <typename T>
std::string fft_global_source()
{
        std::string s;
        s += floating_point_source<T>();
        return s + dft_fft_global_shader;
}

template <typename T>
std::string rows_mul_to_buffer_source()
{
        std::string s;
        s += floating_point_source<T>();
        s += "#define FUNCTION_ROWS_MUL_TO_BUFFER\n\n";
        return s + dft_mul_shader;
}

template <typename T>
std::string rows_mul_fr_buffer_source()
{
        std::string s;
        s += floating_point_source<T>();
        s += "#define FUNCTION_ROWS_MUL_FR_BUFFER\n\n";
        return s + dft_mul_shader;
}

template <typename T>
std::string cols_mul_to_buffer_source()
{
        std::string s;
        s += floating_point_source<T>();
        s += "#define FUNCTION_COLS_MUL_TO_BUFFER\n\n";
        return s + dft_mul_shader;
}

template <typename T>
std::string cols_mul_fr_buffer_source()
{
        std::string s;
        s += floating_point_source<T>();
        s += "#define FUNCTION_COLS_MUL_FR_BUFFER\n\n";
        return s + dft_mul_shader;
}

template <typename T>
std::string rows_mul_d_source()
{
        std::string s;
        s += floating_point_source<T>();
        s += "#define FUNCTION_ROWS_MUL_D\n\n";
        return s + dft_mul_shader;
}

template <typename T>
std::string move_to_input_source()
{
        std::string s;
        s += floating_point_source<T>();
        s += "#define FUNCTION_MOVE_TO_INPUT\n\n";
        return s + dft_copy_shader;
}

template <typename T>
std::string move_to_output_source()
{
        std::string s;
        s += floating_point_source<T>();
        s += "#define FUNCTION_MOVE_TO_OUTPUT\n\n";
        return s + dft_copy_shader;
}

template <typename T>
std::string fft_shared_source(int N, int shared_size, bool reverse_input)
{
        std::string s;
        s += floating_point_source<T>();
        s += "const uint N = " + std::to_string(N) + ";\n";
        s += "const uint N_MASK = " + std::to_string(N - 1) + ";\n";
        s += "const uint N_BITS = " + std::to_string(binary_size(N)) + ";\n";
        s += "const uint SHARED_SIZE = " + std::to_string(shared_size) + ";\n";
        s += "const bool REVERSE_INPUT = " + (reverse_input ? std::string("true") : std::string("false")) + ";\n";
        return s + dft_fft_shared_shader;
}
}

template <typename T>
DeviceProg<T>::DeviceProg()
        : m_bit_reverse(opengl::ComputeShader(bit_reverse_source<T>())),
          m_fft(opengl::ComputeShader(fft_global_source<T>())),
          m_rows_mul_to_buffer(opengl::ComputeShader(rows_mul_to_buffer_source<T>())),
          m_rows_mul_fr_buffer(opengl::ComputeShader(rows_mul_fr_buffer_source<T>())),
          m_cols_mul_to_buffer(opengl::ComputeShader(cols_mul_to_buffer_source<T>())),
          m_cols_mul_fr_buffer(opengl::ComputeShader(cols_mul_fr_buffer_source<T>())),
          m_rows_mul_d(opengl::ComputeShader(rows_mul_d_source<T>())),
          m_move_to_input(opengl::ComputeShader(move_to_input_source<T>())),
          m_move_to_output(opengl::ComputeShader(move_to_output_source<T>()))
{
}

template <typename T>
DeviceProgFFTShared<T>::DeviceProgFFTShared(int N, int shared_size, bool reverse_input, int group_size)
        : m_group_size(group_size),
          m_shared_size(shared_size),
          m_fft(opengl::ComputeShader(fft_shared_source<T>(N, shared_size, reverse_input)))
{
}

template class DeviceProg<float>;
template class DeviceProg<double>;
template class DeviceProgFFTShared<float>;
template class DeviceProgFFTShared<double>;
