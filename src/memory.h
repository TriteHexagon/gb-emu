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

#include <array>
#include <memory>
#include "common.h"
#include "mapper.h"
#include "rom.h"

struct Devices;

class Memory
{
public:
    explicit Memory(Devices& devices) : m_devices(devices)
    {
    }

    void LoadROM(ROMInfo& rom_info);
    void Reset();
    u8 Read(u16 addr);
    void Write(u16 addr, u8 val);

    const std::vector<u8>& GetRAM()
    {
        return m_mapper->GetRAM();
    }

private:
    u8 ReadMMIO(u16 addr);
    u8 Read_Fnnn(u16 addr);
    void WriteMMIO(u16 addr, u8 val);
    void Write_Fnnn(u16 addr, u8 val);

    Devices& m_devices;

    std::array<u8, 0x2000> m_wram; // work RAM
    std::array<u8, 0x7F> m_hram;   // high RAM

    std::unique_ptr<Mapper> m_mapper;
};
