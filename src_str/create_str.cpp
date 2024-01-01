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

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <span>
#include <sstream>
#include <string>
#include <string_view>

constexpr int LINE_LENGTH_STR = 24;
constexpr int LINE_LENGTH_BIN = 16;
constexpr int LINE_LENGTH_SPR = 8;

constexpr std::string_view COMMAND_STR = "str";
constexpr std::string_view COMMAND_BIN = "bin";
constexpr std::string_view COMMAND_SPR = "spr";
constexpr std::string_view COMMAND_CAT = "cat";

// SPIR-V Specification
// 3.1 Magic Number
constexpr std::uint32_t SPR_MAGIC_NUMBER = 0x07230203;

constexpr std::uint32_t bswap32(const std::uint32_t n)
{
        return __builtin_bswap32(n);
}

static_assert(bswap32(0x12345678) == 0x78563412);

namespace
{
[[noreturn]] void error(const std::string_view msg) noexcept
{
        std::cerr << msg << std::endl;
        std::exit(EXIT_FAILURE);
}

[[nodiscard]] std::string usage()
{
        std::ostringstream s;
        s << "Usage:\n";
        s << "program " << COMMAND_STR << "|" << COMMAND_BIN << "|" << COMMAND_SPR << " file_in file_out\n";
        s << "program " << COMMAND_CAT << " files_in file_out";
        return s.str();
}

template <typename T>
[[nodiscard]] int char_to_int(const T c)
{
        static_assert(std::is_same_v<T, char>);
        return static_cast<unsigned char>(c);
}

[[nodiscard]] std::ifstream create_ifstream(
        const char* const name,
        const std::ios_base::openmode mode = std::ios_base::in)
{
        std::ifstream stream(name, mode);
        if (stream)
        {
                return stream;
        }
        error("Error opening input file \"" + std::string(name) + "\"");
}

[[nodiscard]] std::ofstream create_ofstream(
        const char* const name,
        const std::ios_base::openmode mode = std::ios_base::out)
{
        std::ofstream stream(name, mode);
        if (stream)
        {
                return stream;
        }
        error("Error opening output file \"" + std::string(name) + "\"");
}

void str(const char* const input_name, const char* const output_name)
{
        const auto write = [](std::ofstream& ofs, const char c)
        {
                ofs << "\\x" << std::setw(2) << char_to_int(c);
        };

        std::ifstream ifs = create_ifstream(input_name);
        std::ofstream ofs = create_ofstream(output_name);
        ofs << std::hex << std::setfill('0');

        char c;

        ofs << "\"";
        if (ifs.get(c))
        {
                write(ofs, c);
        }
        for (long long i = 1; ifs.get(c); ++i)
        {
                if (i % LINE_LENGTH_STR == 0)
                {
                        ofs << "\"\n\"";
                }
                write(ofs, c);
        }
        ofs << "\"";
        ofs << std::endl;

        if (!ofs)
        {
                error(std::string("Error writing to str file \"") + output_name + "\"");
        }
}

void bin(const char* const input_name, const char* const output_name)
{
        const auto write = [](std::ofstream& ofs, const char c)
        {
                ofs << "0x" << std::setw(2) << char_to_int(c);
        };

        std::ifstream ifs = create_ifstream(input_name, std::ios_base::binary);
        std::ofstream ofs = create_ofstream(output_name);
        ofs << std::hex << std::setfill('0');

        char c;

        if (ifs.get(c))
        {
                write(ofs, c);
        }
        for (long long i = 1; ifs.get(c); ++i)
        {
                ofs << ',' << ((i % LINE_LENGTH_BIN != 0) ? ' ' : '\n');
                write(ofs, c);
        }
        ofs << std::endl;

        if (!ofs)
        {
                error(std::string("Error writing to bin file \"") + output_name + "\"");
        }
}

void spr(const char* const input_name, const char* const output_name)
{
        const auto read = [](std::ifstream& ifs, std::uint32_t* const n)
        {
                return static_cast<bool>(ifs.read(reinterpret_cast<char*>(n), sizeof(*n)));
        };

        const auto write = [](std::ofstream& ofs, const bool reverse_byte_order, const std::uint32_t n)
        {
                ofs << "0x" << std::setw(8) << (!reverse_byte_order ? n : bswap32(n));
        };

        std::ifstream ifs = create_ifstream(input_name, std::ios_base::binary);
        std::ofstream ofs = create_ofstream(output_name);
        ofs << std::hex << std::setfill('0');

        std::uint32_t n;

        if (!read(ifs, &n))
        {
                error("Error reading SPIR-V magic number");
        }

        const bool reverse_byte_order = [n]
        {
                if (n == SPR_MAGIC_NUMBER)
                {
                        return false;
                }
                if (bswap32(n) == SPR_MAGIC_NUMBER)
                {
                        return true;
                }
                error("Error reading SPIR-V (no magic number)");
        }();

        write(ofs, reverse_byte_order, n);
        for (long long i = 1; read(ifs, &n); ++i)
        {
                ofs << ',' << ((i % LINE_LENGTH_SPR != 0) ? ' ' : '\n');
                write(ofs, reverse_byte_order, n);
        }

        if (ifs.gcount() != 0)
        {
                error("Error reading SPIR-V (code size is not a multiple of " + std::to_string(sizeof(n)) + ")");
        }

        ofs << std::endl;

        if (!ofs)
        {
                error(std::string("Error writing to spr file \"") + output_name + "\"");
        }
}

void cat(const std::span<const char*> input_names, const char* const output_name)
{
        std::string s;
        for (const char* const name : input_names)
        {
                std::ifstream ifs = create_ifstream(name, std::ios_base::binary);
                char c;
                while (ifs.get(c))
                {
                        s += c;
                }
        }

        std::ofstream ofs = create_ofstream(output_name, std::ios_base::binary);
        if (!ofs.write(s.data(), s.size()))
        {
                error(std::string("Error writing to cat file \"") + output_name + "\"");
        }
}
}

int main(const int argc, const char* argv[])
{
        if (argc < 2)
        {
                error(usage());
        }

        const std::string_view command(argv[1]);

        if (command == COMMAND_STR)
        {
                if (argc != 4)
                {
                        error(usage());
                }
                str(argv[2], argv[3]);
        }
        else if (command == COMMAND_BIN)
        {
                if (argc != 4)
                {
                        error(usage());
                }
                bin(argv[2], argv[3]);
        }
        else if (command == COMMAND_SPR)
        {
                if (argc != 4)
                {
                        error(usage());
                }
                spr(argv[2], argv[3]);
        }
        else if (command == COMMAND_CAT)
        {
                if (argc < 4)
                {
                        error(usage());
                }
                cat({argv + 2, argv + argc - 1}, argv[argc - 1]);
        }
        else
        {
                error(usage());
        }

        return EXIT_SUCCESS;
}
