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

#include "mbc3.h"

MBC3::MBC3(ROMInfo& rom_info) :
    m_rom(std::move(*rom_info.rom)),
    m_ram(std::move(*rom_info.ram))
{
}

void MBC3::Reset()
{
    m_rom_map = nullptr;
    m_ram_map = nullptr;
    m_rom_bank = 1;
    m_ram_bank = 0;
    m_ram_enable = false;
    UpdateMapping();
}

u8 MBC3::Read(u16 addr)
{
    if (addr < 0x4000)
    {
        return m_rom[addr];
    }
    else if (addr < 0x8000)
    {
        return m_rom_map[addr - 0x4000];
    }
    else if (addr >= 0xA000 && addr < 0xC000 && m_ram.size() != 0 && m_ram_enable)
    {
        return m_ram_map[(addr - 0xA000) & (m_ram.size() - 1)];
    }

    return 0;
}

void MBC3::Write(u16 addr, u8 val)
{
    if (addr < 0x2000)
    {
        m_ram_enable = (val == 0x0A);
    }
    else if (addr < 0x4000)
    {
        val &= 0x7F;
        if (val == 0)
        {
            val = 1;
        }
        m_rom_bank = val;
        UpdateMapping();
    }
    else if (addr < 0x6000)
    {
        m_ram_bank = val & 0x3;
        UpdateMapping();
    }
    else if (addr >= 0xA000 && addr < 0xC000 && m_ram.size() != 0 && m_ram_enable)
    {
        m_ram_map[(addr - 0xA000) & (m_ram.size() - 1)] = val;
    }
}

void MBC3::UpdateMapping()
{
    u32 rom_addr = (m_rom_bank * 0x4000) & (m_rom.size() - 1);
    m_rom_map = &m_rom[rom_addr];

    if (m_ram.size() != 0)
    {
        u32 ram_addr = (m_ram_bank * 0x2000) & (m_ram.size() - 1);
        m_ram_map = &m_ram[ram_addr];
    }
}
