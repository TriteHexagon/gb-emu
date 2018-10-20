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
#include "mappers/plain_rom.h"
#include "mappers/mbc1.h"
#include "mappers/mbc3.h"

void Memory::LoadROM(ROMInfo& rom_info)
{
    switch (rom_info.mapper_type)
    {
    case MapperType::PlainROM:
        m_mapper = std::make_unique<PlainROM>(rom_info);
        break;
    case MapperType::MBC1:
        m_mapper = std::make_unique<MBC1>(rom_info);
        break;
    case MapperType::MBC3:
        m_mapper = std::make_unique<MBC3>(rom_info);
        break;
    }
}

void Memory::Reset()
{
    m_wram = {};
    m_hram = {};
    m_mapper->Reset();
}

u8 Memory::ReadMMIO(u16 addr)
{
    switch (addr)
    {
    case 0xFF00: // JOYP
        return m_hw.joypad.ReadJOYP();
    case 0xFF04: // DIV
        return m_hw.timer.ReadDIV();
    case 0xFF05: // TIMA
        return m_hw.timer.ReadTIMA();
    case 0xFF06: // TMA
        return m_hw.timer.ReadTMA();
    case 0xFF07: // TAC
        return m_hw.timer.ReadTAC();
    case 0xFF0F: // IF
        return m_hw.cpu.ReadIF();
    case 0xFF40: // LCDC
        return m_hw.graphics.ReadLCDC();
    case 0xFF41: // STAT
        return m_hw.graphics.ReadSTAT();
    case 0xFF42: // SCY
        return m_hw.graphics.ReadSCY();
    case 0xFF43: // SCX
        return m_hw.graphics.ReadSCX();
    case 0xFF44: // LY
        return m_hw.graphics.ReadLY();
    case 0xFF45: // LYC
        return m_hw.graphics.ReadLYC();
    case 0xFF47: // BGP
        return m_hw.graphics.ReadBGP();
    case 0xFF48: // OBP0
        return m_hw.graphics.ReadOBP0();
    case 0xFF49: // OBP1
        return m_hw.graphics.ReadOBP1();
    case 0xFF4A: // WY
        return m_hw.graphics.ReadWY();
    case 0xFF4B: // WX
        return m_hw.graphics.ReadWX();
    }

    return 0xFF;
}

u8 Memory::Read_Fnnn(u16 addr)
{
    if (addr == 0xFFFF)
    {
        // 0xFFFF
        return m_hw.cpu.ReadIE();
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
        return m_hw.graphics.ReadOAM(addr - 0xFE00);
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
        return m_mapper->Read(addr);
    case 0x8:
    case 0x9:
        return m_hw.graphics.ReadVRAM(addr & 0x1FFF);
    case 0xA:
    case 0xB:
        return m_mapper->Read(addr);
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
    case 0xFF00: // JOYP
        m_hw.joypad.WriteJOYP(val);
        break;
    case 0xFF01: // SB
        break;
    case 0xFF04: // DIV
        m_hw.timer.WriteDIV();
        break;
    case 0xFF05: // TIMA
        m_hw.timer.WriteTIMA(val);
        break;
    case 0xFF06: // TMA
        m_hw.timer.WriteTMA(val);
        break;
    case 0xFF07: // TAC
        m_hw.timer.WriteTAC(val);
        break;
    case 0xFF0F: // IF
        m_hw.cpu.WriteIF(val);
        break;
    case 0xFF40: // LCDC
        m_hw.graphics.WriteLCDC(val);
        break;
    case 0xFF41: // STAT
        m_hw.graphics.WriteSTAT(val);
        break;
    case 0xFF42: // SCY
        m_hw.graphics.WriteSCY(val);
        break;
    case 0xFF43: // SCX
        m_hw.graphics.WriteSCX(val);
        break;
    case 0xFF45: // LYC
        m_hw.graphics.WriteLYC(val);
        break;
    case 0xFF46: // DMA
        m_hw.graphics.WriteDMA(val);
        break;
    case 0xFF47: // BGP
        m_hw.graphics.WriteBGP(val);
        break;
    case 0xFF48: // OBP0
        m_hw.graphics.WriteOBP0(val);
        break;
    case 0xFF49: // OBP1
        m_hw.graphics.WriteOBP1(val);
        break;
    case 0xFF4A: // WY
        m_hw.graphics.WriteWY(val);
        break;
    case 0xFF4B: // WX
        m_hw.graphics.WriteWX(val);
        break;
    }
}

void Memory::Write_Fnnn(u16 addr, u8 val)
{
    if (addr == 0xFFFF)
    {
        // 0xFFFF
        m_hw.cpu.WriteIE(val);
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
        m_hw.graphics.WriteOAM(addr - 0xFE00, val);
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
        m_mapper->Write(addr, val);
        break;
    case 0x8:
    case 0x9:
        m_hw.graphics.WriteVRAM(addr & 0x1FFF, val);
        break;
    case 0xA:
    case 0xB:
        m_mapper->Write(addr, val);
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
