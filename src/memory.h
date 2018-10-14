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

#pragma once

#include <vector>
#include "common.h"

class Machine;

class Memory
{
public:
    explicit Memory(Machine& machine) : m_machine(machine)
    {
    }

    void LoadROM(std::vector<u8> rom);
    void Reset();
    u8 Read(u16 addr);
    void Write(u16 addr, u8 val);

private:
    u8 ReadMMIO(u16 addr);
    u8 Read_Fnnn(u16 addr);
    void WriteMMIO(u16 addr, u8 val);
    void Write_Fnnn(u16 addr, u8 val);

    Machine& m_machine;

    u8 m_vram[0x2000]; // video RAM
    u8 m_wram[0x2000]; // work RAM
    u8 m_oam[0xA0];    // object attribute memory
    u8 m_hram[0x7F];   // high RAM

    // TODO: implement mappers
    std::vector<u8> m_rom;
    u8 m_sram[0x2000];
};
