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

#include "program.h"

#include "com/bits.h"
#include "com/print.h"

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
constexpr const char dft_copy_input_shader[]
{
#include "dft_copy_input.comp.str"
};
constexpr const char dft_copy_output_shader[]
{
#include "dft_copy_output.comp.str"
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

std::string group_size_string(int group_size)
{
        return "const uint GROUP_SIZE = " + to_string(group_size) + ";\n";
}

std::string group_size_string(vec2i group_size)
{
        return "const uvec2 GROUP_SIZE = uvec2(" + to_string(group_size[0]) + ", " + to_string(group_size[1]) + ");\n";
}

template <typename T>
std::string bit_reverse_source(int group_size)
{
        std::string s;
        s += floating_point_source<T>();
        s += group_size_string(group_size);
        return s + dft_bit_reverse_shader;
}

template <typename T>
std::string fft_global_source(int group_size)
{
        std::string s;
        s += floating_point_source<T>();
        s += group_size_string(group_size);
        return s + dft_fft_global_shader;
}

template <typename T>
std::string rows_mul_to_buffer_source(vec2i group_size)
{
        std::string s;
        s += floating_point_source<T>();
        s += "#define FUNCTION_ROWS_MUL_TO_BUFFER\n\n";
        s += group_size_string(group_size);
        return s + dft_mul_shader;
}

template <typename T>
std::string rows_mul_fr_buffer_source(vec2i group_size)
{
        std::string s;
        s += floating_point_source<T>();
        s += "#define FUNCTION_ROWS_MUL_FR_BUFFER\n\n";
        s += group_size_string(group_size);
        return s + dft_mul_shader;
}

template <typename T>
std::string cols_mul_to_buffer_source(vec2i group_size)
{
        std::string s;
        s += floating_point_source<T>();
        s += "#define FUNCTION_COLS_MUL_TO_BUFFER\n\n";
        s += group_size_string(group_size);
        return s + dft_mul_shader;
}

template <typename T>
std::string cols_mul_fr_buffer_source(vec2i group_size)
{
        std::string s;
        s += floating_point_source<T>();
        s += "#define FUNCTION_COLS_MUL_FR_BUFFER\n\n";
        s += group_size_string(group_size);
        return s + dft_mul_shader;
}

template <typename T>
std::string rows_mul_d_source(vec2i group_size)
{
        std::string s;
        s += floating_point_source<T>();
        s += "#define FUNCTION_ROWS_MUL_D\n\n";
        s += group_size_string(group_size);
        return s + dft_mul_shader;
}

template <typename T>
std::string copy_input_source(vec2i group_size)
{
        std::string s;
        s += floating_point_source<T>();
        s += group_size_string(group_size);
        return s + dft_copy_input_shader;
}

template <typename T>
std::string copy_output_source(vec2i group_size)
{
        std::string s;
        s += floating_point_source<T>();
        s += group_size_string(group_size);
        return s + dft_copy_output_shader;
}

template <typename T>
std::string fft_shared_source(int n, int n_bits, int shared_size, int group_size, bool reverse_input)
{
        std::string s;
        s += floating_point_source<T>();
        s += "const uint N = " + to_string(n) + ";\n";
        s += "const uint N_MASK = " + to_string(n - 1) + ";\n";
        s += "const uint N_BITS = " + to_string(n_bits) + ";\n";
        s += "const uint SHARED_SIZE = " + to_string(shared_size) + ";\n";
        s += "const uint GROUP_SIZE = " + to_string(group_size) + ";\n";
        s += "const bool REVERSE_INPUT = " + (reverse_input ? std::string("true") : std::string("false")) + ";\n";
        return s + dft_fft_shared_shader;
}
}

template <typename T>
DeviceProg<T>::DeviceProg(int group_size_1d, vec2i group_size_2d)
        : m_group_size_1d(group_size_1d),
          m_group_size_2d(group_size_2d),
          m_bit_reverse(opengl::ComputeShader(bit_reverse_source<T>(group_size_1d))),
          m_fft(opengl::ComputeShader(fft_global_source<T>(group_size_1d))),
          m_rows_mul_to_buffer(opengl::ComputeShader(rows_mul_to_buffer_source<T>(group_size_2d))),
          m_rows_mul_fr_buffer(opengl::ComputeShader(rows_mul_fr_buffer_source<T>(group_size_2d))),
          m_cols_mul_to_buffer(opengl::ComputeShader(cols_mul_to_buffer_source<T>(group_size_2d))),
          m_cols_mul_fr_buffer(opengl::ComputeShader(cols_mul_fr_buffer_source<T>(group_size_2d))),
          m_rows_mul_d(opengl::ComputeShader(rows_mul_d_source<T>(group_size_2d))),
          m_copy_input(opengl::ComputeShader(copy_input_source<T>(group_size_2d))),
          m_copy_output(opengl::ComputeShader(copy_output_source<T>(group_size_2d)))
{
}

template <typename T>
DeviceProgFFTShared<T>::DeviceProgFFTShared(int n, int shared_size, int group_size, bool reverse_input)
        : m_n(n),
          m_n_bits(binary_size(n)),
          m_shared_size(shared_size),
          m_fft(opengl::ComputeShader(fft_shared_source<T>(m_n, m_n_bits, m_shared_size, group_size, reverse_input)))
{
        ASSERT((1 << m_n_bits) == m_n);
}

template class DeviceProg<float>;
template class DeviceProg<double>;
template class DeviceProgFFTShared<float>;
template class DeviceProgFFTShared<double>;
