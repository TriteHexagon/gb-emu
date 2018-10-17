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

#include "mbc1.h"

MBC1::MBC1(ROMInfo& rom_info) :
    m_rom(std::move(*rom_info.rom)),
    m_ram(rom_info.ram_size, 0)
{
}

void MBC1::Reset()
{
    m_bank_reg1 = 1;
    m_bank_reg2 = 0;
    m_ram_enable = false;
    m_ram_banking_mode = false;
    UpdateMapping();
}

u8 MBC1::Read(u16 addr)
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

void MBC1::Write(u16 addr, u8 val)
{
    if (addr < 0x2000)
    {
        m_ram_enable = (val == 0x0A);
    }
    else if (addr < 0x4000)
    {
        val &= 0x1F;
        if (val == 0)
        {
            val = 1;
        }
        m_bank_reg1 = val;
        UpdateMapping();
    }
    else if (addr < 0x6000)
    {
        m_bank_reg2 = val & 0x3;
        UpdateMapping();
    }
    else if (addr < 0x8000)
    {
        m_ram_banking_mode = (bool)(val & 1);
        UpdateMapping();
    }
    else if (addr >= 0xA000 && addr < 0xC000 && m_ram.size() != 0 && m_ram_enable)
    {
        m_ram_map[(addr - 0xA000) & (m_ram.size() - 1)] = val;
    }
}

int MBC1::GetROMBank()
{
    if (m_ram_banking_mode)
    {
        return m_bank_reg1;
    }
    else
    {
        return (m_bank_reg2 << 5) | m_bank_reg1;
    }
}

int MBC1::GetRAMBank()
{
    if (m_ram_banking_mode)
    {
        return m_bank_reg2;
    }
    else
    {
        return 0;
    }
}

void MBC1::UpdateMapping()
{
    u32 rom_addr = (GetROMBank() * 0x4000) & (m_rom.size() - 1);
    m_rom_map = &m_rom[rom_addr];
    u32 ram_addr = (GetRAMBank() * 0x2000) & (m_ram.size() - 1);
    m_ram_map = &m_ram[ram_addr];
}
