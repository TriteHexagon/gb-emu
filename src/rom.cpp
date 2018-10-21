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
#include "binary_file_reader.h"
#include "binary_file_writer.h"

LoadROMStatus LoadROM(const std::string& file_name, ROMInfo& info)
{
    static const std::array<u32, 6> ram_size_table = { 0, 0x800, 0x2000, 0x8000, 0x20000, 0x10000 };
    const long min_rom_size = 0x8000; // 32KB
    const long max_rom_size = 0x800000; // 8MB

    // ROM header offsets
    const int ofs_cgb_flag = 0x143;
    const int ofs_cart_type = 0x147;
    const int ofs_rom_size = 0x148;
    const int ofs_ram_size = 0x149;


    BinaryFileReader rom_file_reader(file_name);

    if (!rom_file_reader.IsOpen())
    {
        return LoadROMStatus::ROMFileOpenFailed;
    }

    long rom_size = rom_file_reader.GetSize();

    if (rom_size < min_rom_size)
    {
        return LoadROMStatus::ROMTooSmall;
    }

    if (rom_size > max_rom_size)
    {
        return LoadROMStatus::ROMTooLarge;
    }

    if (rom_size & (rom_size - 1))
    {
        return LoadROMStatus::ROMSizeNotPowerOfTwo;
    }

    info.rom = std::make_unique<std::vector<u8>>(rom_size, 0);

    if (!rom_file_reader.ReadBytes(info.rom->data(), rom_size))
    {
        return LoadROMStatus::ROMFileReadFailed;
    }
    
    info.is_cgb_aware = (((*info.rom)[ofs_cgb_flag] & 0x80) != 0);

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
    case 0x10:
        info.mapper_type = MapperType::MBC3;
        has_ram = true;
        info.has_battery = true;
        info.has_rtc = true;
        break;
    case 0x11:
        info.mapper_type = MapperType::MBC3;
        break;
    case 0x12:
        info.mapper_type = MapperType::MBC3;
        has_ram = true;
        break;
    case 0x13:
        info.mapper_type = MapperType::MBC3;
        has_ram = true;
        info.has_battery = true;
        break;
    case 0x19:
        info.mapper_type = MapperType::MBC5;
        break;
    case 0x1A:
        info.mapper_type = MapperType::MBC5;
        has_ram = true;
        break;
    case 0x1B:
        info.mapper_type = MapperType::MBC5;
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

    u32 ram_size = ram_size_table[info.ram_size_index];

    if (has_ram != (ram_size != 0))
    {
        return LoadROMStatus::RAMInfoInconsistent;
    }

    info.ram = std::make_unique<std::vector<u8>>(ram_size, 0);

    if (info.has_battery)
    {
        size_t slash_pos = file_name.find_last_of('/');

#ifdef _WIN32
        size_t backslash_pos = file_name.find_last_of('\\');

        if (slash_pos == std::string::npos
            || (backslash_pos != std::string::npos && backslash_pos > slash_pos))
        {
            slash_pos = backslash_pos;
        }
#endif // _WIN32

        size_t dot_pos = file_name.find_last_of('.');

        if (dot_pos != std::string::npos && (slash_pos == std::string::npos || slash_pos < dot_pos))
        {
            info.save_file_name = file_name.substr(0, dot_pos) + ".sav";
        }
        else
        {
            info.save_file_name = file_name + ".sav";
        }

        BinaryFileReader save_file_reader(info.save_file_name);

        if (save_file_reader.IsOpen())
        {
            if (save_file_reader.GetSize() != ram_size)
            {
                return LoadROMStatus::RAMFileWrongSize;
            }

            if (!save_file_reader.ReadBytes(info.ram->data(), ram_size))
            {
                return LoadROMStatus::RAMFileReadFailed;
            }
        }
    }

    return LoadROMStatus::OK;
}

SaveRAMStatus SaveRAM(const std::string& file_name, const std::vector<u8>& ram)
{
    BinaryFileWriter writer(file_name);

    if (!writer.IsOpen())
    {
        return SaveRAMStatus::FileOpenFailed;
    }

    if (!writer.WriteBytes(ram.data(), ram.size()))
    {
        return SaveRAMStatus::FileWriteFailed;
    }

    return SaveRAMStatus::OK;
}
