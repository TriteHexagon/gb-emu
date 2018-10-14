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
#include <string.h>
#include <algorithm>
#include "common.h"
#include "memory.h"
#include "cpu.h"
#include "machine.h"

void Memory::LoadROM(std::vector<u8> rom)
{
    m_rom = std::move(rom);
}

void Memory::Reset()
{
    memset(m_vram, 0, sizeof(m_vram));
    memset(m_wram, 0, sizeof(m_wram));
    memset(m_oam, 0, sizeof(m_oam));
    memset(m_hram, 0, sizeof(m_hram));
}

u8 Memory::ReadMMIO(u16 addr)
{
    switch (addr)
    {
    case 0xFF04: // DIV
        return m_machine.GetTimer().ReadDIV();
    case 0xFF05: // TIMA
        return m_machine.GetTimer().ReadTIMA();
    case 0xFF06: // TMA
        return m_machine.GetTimer().ReadTMA();
    case 0xFF07: // TAC
        return m_machine.GetTimer().ReadTAC();
    case 0xFF0F: // IF
        return m_machine.GetCPU().ReadIF();
    }

    return 0xFF;
}

u8 Memory::Read_Fnnn(u16 addr)
{
    if (addr == 0xFFFF)
    {
        // 0xFFFF
        return m_machine.GetCPU().ReadIE();
    }
    else if (addr >= 0xFF80)
    {
        // 0xFF80-0xFFFE
        return m_hram[addr - 0xFF80];
    }
    else if (addr >= 0xFF00)
    {
        // 0xFF00-0xFF7F
        return ReadMMIO(addr);
    }
    else if (addr >= 0xFEA0)
    {
        // 0xFEA0-0xFEFF
        // DMG behavior is to read 0
        return 0;
    }
    else if (addr >= 0xFE00)
    {
        // 0xFE00-0xFE9F
        return m_oam[addr - 0xFE00];
    }
    else
    {
        // 0xF000-0xFDFF
        return m_wram[addr & 0x1FFF];
    }
}

u8 Memory::Read(u16 addr)
{
    switch (addr >> 12)
    {
    case 0x0:
    case 0x1:
    case 0x2:
    case 0x3:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
        return m_rom[addr];
    case 0x8:
    case 0x9:
        return m_vram[addr & 0x1FFF];
    case 0xA:
    case 0xB:
        return m_sram[addr & 0x1FFF];
    case 0xC:
    case 0xD:
    case 0xE:
        return m_wram[addr & 0x1FFF];
    case 0xF:
        return Read_Fnnn(addr);
    }

    return 0; // unreachable
}

void Memory::WriteMMIO(u16 addr, u8 val)
{
    switch (addr)
    {
    case 0xFF01: // SB
        putchar(val); // for CPU test output
        break;
    case 0xFF04: // DIV
        m_machine.GetTimer().WriteDIV();
        break;
    case 0xFF05: // TIMA
        m_machine.GetTimer().WriteTIMA(val);
        break;
    case 0xFF06: // TMA
        m_machine.GetTimer().WriteTMA(val);
        break;
    case 0xFF07: // TAC
        m_machine.GetTimer().WriteTAC(val);
        break;
    case 0xFF0F: // IF
        m_machine.GetCPU().WriteIF(val);
        break;
    }
}

void Memory::Write_Fnnn(u16 addr, u8 val)
{
    if (addr == 0xFFFF)
    {
        // 0xFFFF
        m_machine.GetCPU().WriteIE(val);
    }
    else if (addr >= 0xFF80)
    {
        // 0xFF80-0xFFFE
        m_hram[addr - 0xFF80] = val;
    }
    else if (addr >= 0xFF00)
    {
        // 0xFF00-0xFF7F
        WriteMMIO(addr, val);
    }
    else if (addr >= 0xFEA0)
    {
        // 0xFEA0-0xFEFF
        // DMG behavior is to ignore writes
    }
    else if (addr >= 0xFE00)
    {
        // 0xFE00-0xFE9F
        m_oam[addr - 0xFE00] = val;
    }
    else
    {
        // 0xF000-0xFDFF
        m_wram[addr & 0x1FFF] = val;
    }
}

void Memory::Write(u16 addr, u8 val)
{
    switch (addr >> 12)
    {
    case 0x0:
    case 0x1:
    case 0x2:
    case 0x3:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
        m_rom[addr] = val;
        break;
    case 0x8:
    case 0x9:
        m_vram[addr & 0x1FFF] = val;
        break;
    case 0xA:
    case 0xB:
        m_sram[addr & 0x1FFF] = val;
        break;
    case 0xC:
    case 0xD:
    case 0xE:
        m_wram[addr & 0x1FFF] = val;
        break;
    case 0xF:
        Write_Fnnn(addr, val);
        break;
    }
}
