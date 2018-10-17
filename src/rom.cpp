// Copyright 2018 David Brotz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <stdio.h>
#include <array>
#include "common.h"
#include "rom.h"

LoadROMStatus LoadROM(std::string file_name, ROMInfo& info)
{
    static const std::array<u32, 6> ram_size_table = { 0, 0x800, 0x2000, 0x8000, 0x20000, 0x10000 };
    const long min_rom_size = 0x8000; // 32KB
    const long max_rom_size = 0x800000; // 8MB

    // ROM header offsets
    const int ofs_gbc_flag = 0x143;
    const int ofs_sgb_flag = 0x146;
    const int ofs_cart_type = 0x147;
    const int ofs_rom_size = 0x148;
    const int ofs_ram_size = 0x149;

    FILE* rom_file = fopen(file_name.c_str(), "rb");

    if (rom_file == nullptr)
    {
        return LoadROMStatus::ROMFileOpenFailed;
    }

    fseek(rom_file, 0, SEEK_END);
    long rom_size = ftell(rom_file);

    if (rom_size < min_rom_size)
    {
        fclose(rom_file);
        return LoadROMStatus::ROMTooSmall;
    }

    if (rom_size > max_rom_size)
    {
        fclose(rom_file);
        return LoadROMStatus::ROMTooLarge;
    }

    if (rom_size & (rom_size - 1))
    {
        fclose(rom_file);
        return LoadROMStatus::ROMSizeNotPowerOfTwo;
    }

    info.rom = std::make_unique<std::vector<u8>>(rom_size, 0);

    rewind(rom_file);

    if (fread(info.rom->data(), rom_size, 1, rom_file) != 1)
    {
        fclose(rom_file);
        return LoadROMStatus::ROMFileReadFailed;
    }

    fclose(rom_file);

    info.is_gbc_aware = (((*info.rom)[ofs_gbc_flag] & 0x80) != 0);
    info.is_sgb_aware = ((*info.rom)[ofs_sgb_flag] == 0x03);

    bool has_ram = false;
    info.has_battery = false;
    info.has_rtc = false;

    info.cart_type = (*info.rom)[ofs_cart_type];

    switch (info.cart_type)
    {
    case 0x00:
        info.mapper_type = MapperType::PlainROM;
        break;
    case 0x01:
        info.mapper_type = MapperType::MBC1;
        break;
    case 0x02:
        info.mapper_type = MapperType::MBC1;
        has_ram = true;
        break;
    case 0x03:
        info.mapper_type = MapperType::MBC1;
        has_ram = true;
        info.has_battery = true;
        break;
    case 0x08:
        info.mapper_type = MapperType::PlainROM;
        has_ram = true;
        break;
    case 0x09:
        info.mapper_type = MapperType::PlainROM;
        has_ram = true;
        info.has_battery = true;
        break;
    case 0x11:
        info.mapper_type = MapperType::MBC1;
        break;
    case 0x12:
        info.mapper_type = MapperType::MBC1;
        has_ram = true;
        break;
    case 0x13:
        info.mapper_type = MapperType::MBC1;
        has_ram = true;
        info.has_battery = true;
        break;
    default:
        return LoadROMStatus::UnknownCartridgeType;
    }

    int rom_size_shift = (*info.rom)[ofs_rom_size];

    if (rom_size_shift > 8 || rom_size != (0x8000L << rom_size_shift))
    {
        return LoadROMStatus::ROMSizeMismatch;
    }

    info.ram_size_index = (*info.rom)[ofs_ram_size];

    if (info.ram_size_index >= ram_size_table.size())
    {
        return LoadROMStatus::UnknownRAMSize;
    }

    info.ram_size = ram_size_table[info.ram_size_index];

    if (has_ram != (info.ram_size != 0))
    {
        return LoadROMStatus::RAMInfoInconsistent;
    }

    return LoadROMStatus::OK;
}
