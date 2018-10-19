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

#include "plain_rom.h"

PlainROM::PlainROM(ROMInfo& rom_info) :
    m_rom(std::move(*rom_info.rom)),
    m_ram(std::move(*rom_info.ram))
{
}

void PlainROM::Reset()
{
}

u8 PlainROM::Read(u16 addr)
{
    if (addr < 0x8000)
    {
        return m_rom[addr];
    }
    else if (addr >= 0xA000 && addr < 0xC000 && m_ram.size() != 0)
    {
        return m_ram[(addr - 0xA000) & (m_ram.size() - 1)];
    }

    return 0;
}

void PlainROM::Write(u16 addr, u8 val)
{
    if (addr >= 0xA000 && addr < 0xC000 && m_ram.size() != 0)
    {
        m_ram[(addr - 0xA000) & (m_ram.size() - 1)] = val;
    }
}
