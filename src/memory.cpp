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
#include "mappers/mbc5.h"

const u16 mmio_addr_joyp = 0xFF00;
const u16 mmio_addr_sb = 0xFF01;
const u16 mmio_addr_sc = 0xFF02;
const u16 mmio_addr_div = 0xFF04;
const u16 mmio_addr_tima = 0xFF05;
const u16 mmio_addr_tma = 0xFF06;
const u16 mmio_addr_tac = 0xFF07;
const u16 mmio_addr_if = 0xFF0F;
const u16 mmio_addr_nr10 = 0xFF10;
const u16 mmio_addr_nr11 = 0xFF11;
const u16 mmio_addr_nr12 = 0xFF12;
const u16 mmio_addr_nr13 = 0xFF13;
const u16 mmio_addr_nr14 = 0xFF14;
const u16 mmio_addr_nr21 = 0xFF16;
const u16 mmio_addr_nr22 = 0xFF17;
const u16 mmio_addr_nr23 = 0xFF18;
const u16 mmio_addr_nr24 = 0xFF19;
const u16 mmio_addr_nr30 = 0xFF1A;
const u16 mmio_addr_nr31 = 0xFF1B;
const u16 mmio_addr_nr32 = 0xFF1C;
const u16 mmio_addr_nr33 = 0xFF1D;
const u16 mmio_addr_nr34 = 0xFF1E;
const u16 mmio_addr_nr41 = 0xFF20;
const u16 mmio_addr_nr42 = 0xFF21;
const u16 mmio_addr_nr43 = 0xFF22;
const u16 mmio_addr_nr44 = 0xFF23;
const u16 mmio_addr_nr50 = 0xFF24;
const u16 mmio_addr_nr51 = 0xFF25;
const u16 mmio_addr_nr52 = 0xFF26;
const u16 mmio_addr_wave = 0xFF30;
const u16 mmio_addr_lcdc = 0xFF40;
const u16 mmio_addr_stat = 0xFF41;
const u16 mmio_addr_scy = 0xFF42;
const u16 mmio_addr_scx = 0xFF43;
const u16 mmio_addr_ly = 0xFF44;
const u16 mmio_addr_lyc = 0xFF45;
const u16 mmio_addr_dma = 0xFF46;
const u16 mmio_addr_bgp = 0xFF47;
const u16 mmio_addr_obp0 = 0xFF48;
const u16 mmio_addr_obp1 = 0xFF49;
const u16 mmio_addr_wy = 0xFF4A;
const u16 mmio_addr_wx = 0xFF4B;
const u16 mmio_addr_key1 = 0xFF4D;
const u16 mmio_addr_vbk = 0xFF4F;
const u16 mmio_addr_hdma1 = 0xFF51;
const u16 mmio_addr_hdma2 = 0xFF52;
const u16 mmio_addr_hdma3 = 0xFF53;
const u16 mmio_addr_hdma4 = 0xFF54;
const u16 mmio_addr_hdma5 = 0xFF55;
const u16 mmio_addr_rp = 0xFF56;
const u16 mmio_addr_bcps = 0xFF68;
const u16 mmio_addr_bcpd = 0xFF69;
const u16 mmio_addr_ocps = 0xFF6A;
const u16 mmio_addr_ocpd = 0xFF6B;
const u16 mmio_addr_svbk = 0xFF70;
const u16 mmio_addr_ie = 0xFFFF;

const unsigned int svbk_mask = 0x7;

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
    case MapperType::MBC5:
        m_mapper = std::make_unique<MBC5>(rom_info);
        break;
    }
}

void Memory::Reset()
{
    m_wram = {};
    m_hram = {};
    m_wram_map = &m_wram[0x1000];
    m_wram_bank = 0;
    m_mapper->Reset();
}

u8 Memory::ReadSVBK()
{
    return m_wram_bank | ~svbk_mask;
}

void Memory::WriteSVBK(u8 val)
{
    m_wram_bank = val & svbk_mask;
    int bank = (m_wram_bank == 0) ? 1 : m_wram_bank;
    m_wram_map = &m_wram[bank * 0x1000];
}

u8 Memory::ReadMMIO(u16 addr)
{
    switch (addr)
    {
    case mmio_addr_joyp:
        return m_hw.joypad.ReadJOYP();
    case mmio_addr_div:
        return m_hw.timer.ReadDIV();
    case mmio_addr_tima:
        return m_hw.timer.ReadTIMA();
    case mmio_addr_tma:
        return m_hw.timer.ReadTMA();
    case mmio_addr_tac:
        return m_hw.timer.ReadTAC();
    case mmio_addr_if:
        return m_hw.cpu.ReadIF();
    case mmio_addr_nr10:
        return m_hw.audio.ReadNR10();
    case mmio_addr_nr11:
        return m_hw.audio.ReadNR11();
    case mmio_addr_nr12:
        return m_hw.audio.ReadNR12();
    case mmio_addr_nr13:
        return m_hw.audio.ReadNR13();
    case mmio_addr_nr14:
        return m_hw.audio.ReadNR14();
    case mmio_addr_nr21:
        return m_hw.audio.ReadNR21();
    case mmio_addr_nr22:
        return m_hw.audio.ReadNR22();
    case mmio_addr_nr23:
        return m_hw.audio.ReadNR23();
    case mmio_addr_nr24:
        return m_hw.audio.ReadNR24();
    case mmio_addr_nr30:
        return m_hw.audio.ReadNR30();
    case mmio_addr_nr31:
        return m_hw.audio.ReadNR31();
    case mmio_addr_nr32:
        return m_hw.audio.ReadNR32();
    case mmio_addr_nr33:
        return m_hw.audio.ReadNR33();
    case mmio_addr_nr34:
        return m_hw.audio.ReadNR34();
    case mmio_addr_nr41:
        return m_hw.audio.ReadNR41();
    case mmio_addr_nr42:
        return m_hw.audio.ReadNR42();
    case mmio_addr_nr43:
        return m_hw.audio.ReadNR43();
    case mmio_addr_nr44:
        return m_hw.audio.ReadNR44();
    case mmio_addr_nr50:
        return m_hw.audio.ReadNR50();
    case mmio_addr_nr51:
        return m_hw.audio.ReadNR51();
    case mmio_addr_nr52:
        return m_hw.audio.ReadNR52();
    case mmio_addr_wave + 0x0:
    case mmio_addr_wave + 0x1:
    case mmio_addr_wave + 0x2:
    case mmio_addr_wave + 0x3:
    case mmio_addr_wave + 0x4:
    case mmio_addr_wave + 0x5:
    case mmio_addr_wave + 0x6:
    case mmio_addr_wave + 0x7:
    case mmio_addr_wave + 0x8:
    case mmio_addr_wave + 0x9:
    case mmio_addr_wave + 0xA:
    case mmio_addr_wave + 0xB:
    case mmio_addr_wave + 0xC:
    case mmio_addr_wave + 0xD:
    case mmio_addr_wave + 0xE:
    case mmio_addr_wave + 0xF:
        return m_hw.audio.ReadWaveRAM(addr & 0xF);
    case mmio_addr_lcdc:
        return m_hw.graphics.ReadLCDC();
    case mmio_addr_stat:
        return m_hw.graphics.ReadSTAT();
    case mmio_addr_scy:
        return m_hw.graphics.ReadSCY();
    case mmio_addr_scx:
        return m_hw.graphics.ReadSCX();
    case mmio_addr_ly:
        return m_hw.graphics.ReadLY();
    case mmio_addr_lyc:
        return m_hw.graphics.ReadLYC();
    case mmio_addr_bgp:
        return m_hw.graphics.ReadBGP();
    case mmio_addr_obp0:
        return m_hw.graphics.ReadOBP0();
    case mmio_addr_obp1:
        return m_hw.graphics.ReadOBP1();
    case mmio_addr_wy:
        return m_hw.graphics.ReadWY();
    case mmio_addr_wx:
        return m_hw.graphics.ReadWX();
    }

    if (m_hw.is_cgb_mode)
    {
        switch (addr)
        {
        case mmio_addr_key1:
            return m_hw.cpu.ReadKEY1();
        case mmio_addr_vbk:
            return m_hw.graphics.ReadVBK();
        case mmio_addr_hdma5:
            return m_hw.graphics.ReadHDMA5();
        case mmio_addr_bcps:
            return m_hw.graphics.ReadBCPS();
        case mmio_addr_bcpd:
            return m_hw.graphics.ReadBCPD();
        case mmio_addr_ocps:
            return m_hw.graphics.ReadOCPS();
        case mmio_addr_ocpd:
            return m_hw.graphics.ReadOCPD();
        case mmio_addr_svbk:
            return ReadSVBK();
        }
    }

    return 0xFF;
}

u8 Memory::Read_Fnnn(u16 addr)
{
    if (addr == mmio_addr_ie)
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
        return m_wram[addr & 0xFFF];
    case 0xD:
        return m_wram_map[addr & 0xFFF];
    case 0xE:
        return m_wram[addr & 0xFFF];
    case 0xF:
        return Read_Fnnn(addr);
    }

    return 0; // unreachable
}

void Memory::WriteMMIO(u16 addr, u8 val)
{
    switch (addr)
    {
    case mmio_addr_joyp:
        m_hw.joypad.WriteJOYP(val);
        break;
    case mmio_addr_div:
        m_hw.timer.WriteDIV();
        break;
    case mmio_addr_tima:
        m_hw.timer.WriteTIMA(val);
        break;
    case mmio_addr_tma:
        m_hw.timer.WriteTMA(val);
        break;
    case mmio_addr_tac:
        m_hw.timer.WriteTAC(val);
        break;
    case mmio_addr_if:
        m_hw.cpu.WriteIF(val);
        break;
    case mmio_addr_nr10:
        m_hw.audio.WriteNR10(val);
        break;
    case mmio_addr_nr11:
        m_hw.audio.WriteNR11(val);
        break;
    case mmio_addr_nr12:
        m_hw.audio.WriteNR12(val);
        break;
    case mmio_addr_nr13:
        m_hw.audio.WriteNR13(val);
        break;
    case mmio_addr_nr14:
        m_hw.audio.WriteNR14(val);
        break;
    case mmio_addr_nr21:
        m_hw.audio.WriteNR21(val);
        break;
    case mmio_addr_nr22:
        m_hw.audio.WriteNR22(val);
        break;
    case mmio_addr_nr23:
        m_hw.audio.WriteNR23(val);
        break;
    case mmio_addr_nr24:
        m_hw.audio.WriteNR24(val);
        break;
    case mmio_addr_nr30:
        m_hw.audio.WriteNR30(val);
        break;
    case mmio_addr_nr31:
        m_hw.audio.WriteNR31(val);
        break;
    case mmio_addr_nr32:
        m_hw.audio.WriteNR32(val);
        break;
    case mmio_addr_nr33:
        m_hw.audio.WriteNR33(val);
        break;
    case mmio_addr_nr34:
        m_hw.audio.WriteNR34(val);
        break;
    case mmio_addr_nr41:
        m_hw.audio.WriteNR41(val);
        break;
    case mmio_addr_nr42:
        m_hw.audio.WriteNR42(val);
        break;
    case mmio_addr_nr43:
        m_hw.audio.WriteNR43(val);
        break;
    case mmio_addr_nr44:
        m_hw.audio.WriteNR44(val);
        break;
    case mmio_addr_nr50:
        m_hw.audio.WriteNR50(val);
        break;
    case mmio_addr_nr51:
        m_hw.audio.WriteNR51(val);
        break;
    case mmio_addr_nr52:
        m_hw.audio.WriteNR52(val);
        break;
    case mmio_addr_wave + 0x0:
    case mmio_addr_wave + 0x1:
    case mmio_addr_wave + 0x2:
    case mmio_addr_wave + 0x3:
    case mmio_addr_wave + 0x4:
    case mmio_addr_wave + 0x5:
    case mmio_addr_wave + 0x6:
    case mmio_addr_wave + 0x7:
    case mmio_addr_wave + 0x8:
    case mmio_addr_wave + 0x9:
    case mmio_addr_wave + 0xA:
    case mmio_addr_wave + 0xB:
    case mmio_addr_wave + 0xC:
    case mmio_addr_wave + 0xD:
    case mmio_addr_wave + 0xE:
    case mmio_addr_wave + 0xF:
        m_hw.audio.WriteWaveRAM(addr & 0xF, val);
        break;
    case mmio_addr_lcdc:
        m_hw.graphics.WriteLCDC(val);
        break;
    case mmio_addr_stat:
        m_hw.graphics.WriteSTAT(val);
        break;
    case mmio_addr_scy:
        m_hw.graphics.WriteSCY(val);
        break;
    case mmio_addr_scx:
        m_hw.graphics.WriteSCX(val);
        break;
    case mmio_addr_lyc:
        m_hw.graphics.WriteLYC(val);
        break;
    case mmio_addr_dma:
        m_hw.graphics.WriteDMA(val);
        break;
    case mmio_addr_bgp:
        m_hw.graphics.WriteBGP(val);
        break;
    case mmio_addr_obp0:
        m_hw.graphics.WriteOBP0(val);
        break;
    case mmio_addr_obp1:
        m_hw.graphics.WriteOBP1(val);
        break;
    case mmio_addr_wy:
        m_hw.graphics.WriteWY(val);
        break;
    case mmio_addr_wx:
        m_hw.graphics.WriteWX(val);
        break;
    }

    if (m_hw.is_cgb_mode)
    {
        switch (addr)
        {
        case mmio_addr_key1:
            m_hw.cpu.WriteKEY1(val);
            break;
        case mmio_addr_vbk:
            m_hw.graphics.WriteVBK(val);
            break;
        case mmio_addr_hdma1:
            m_hw.graphics.WriteHDMA1(val);
            break;
        case mmio_addr_hdma2:
            m_hw.graphics.WriteHDMA2(val);
            break;
        case mmio_addr_hdma3:
            m_hw.graphics.WriteHDMA3(val);
            break;
        case mmio_addr_hdma4:
            m_hw.graphics.WriteHDMA4(val);
            break;
        case mmio_addr_hdma5:
            m_hw.graphics.WriteHDMA5(val);
            break;
        case mmio_addr_bcps:
            m_hw.graphics.WriteBCPS(val);
            break;
        case mmio_addr_bcpd:
            m_hw.graphics.WriteBCPD(val);
            break;
        case mmio_addr_ocps:
            m_hw.graphics.WriteOCPS(val);
            break;
        case mmio_addr_ocpd:
            m_hw.graphics.WriteOCPD(val);
            break;
        case mmio_addr_svbk:
            WriteSVBK(val);
            break;
        }
    }
}

void Memory::Write_Fnnn(u16 addr, u8 val)
{
    if (addr == mmio_addr_ie)
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
        m_wram[addr & 0xFFF] = val;
        break;
    case 0xD:
        m_wram_map[addr & 0xFFF] = val;
        break;
    case 0xE:
        m_wram[addr & 0xFFF] = val;
        break;
    case 0xF:
        Write_Fnnn(addr, val);
        break;
    }
}
