/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "device.h"

#if defined(__linux__)

#include <src/com/error.h>

#include <cstddef>
#include <fstream>
#include <ios>
#include <span>
#include <string>

namespace ns
{
void read_system_random(const std::span<std::byte> bytes)
{
        constexpr const char* DEV_RANDOM = "/dev/urandom";

        std::ifstream rnd(DEV_RANDOM, std::ios_base::binary);
        if (!rnd)
        {
                error(std::string("error opening file ") + DEV_RANDOM);
        }

        rnd.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
        if (!rnd)
        {
                error(std::string("error reading from file ") + DEV_RANDOM);
        }
}
}

#elif defined(_WIN32)

#include <src/com/error.h>

// first windows.h
#include <windows.h>
// second wincrypt.h
#include <wincrypt.h>

namespace ns
{
namespace
{
class Provider final
{
        HCRYPTPROV hProvider_;

public:
        Provider()
        {
                if (!::CryptAcquireContext(
                            &hProvider_, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
                {
                        error("error CryptAcquireContext");
                }
        }

        ~Provider()
        {
                if (!::CryptReleaseContext(hProvider_, 0))
                {
                        error("error CryptReleaseContext");
                }
        }

        operator HCRYPTPROV() const&
        {
                return hProvider_;
        }

        operator HCRYPTPROV() const&& = delete;

        Provider(const Provider&) = delete;
        Provider& operator=(const Provider&) = delete;
        Provider(Provider&&) = delete;
        Provider& operator=(Provider&&) = delete;
};
}

void read_system_random(const std::span<std::byte> bytes)
{
        Provider provider;
        if (!::CryptGenRandom(provider, bytes.size(), reinterpret_cast<BYTE*>(bytes.data())))
        {
                error("error CryptGenRandom");
        }
}
}

#else

#error This operating system is not supported

#endif
