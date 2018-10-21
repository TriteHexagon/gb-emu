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

#include "common.h"

struct Hardware;

class Audio
{
public:
    explicit Audio(Hardware& hw) : m_hw(hw)
    {
    }

    void Reset();

    u8 ReadNR10();
    void WriteNR10(u8 val);

    u8 ReadNR11();
    void WriteNR11(u8 val);

    u8 ReadNR12();
    void WriteNR12(u8 val);

    u8 ReadNR13();
    void WriteNR13(u8 val);

    u8 ReadNR14();
    void WriteNR14(u8 val);

    u8 ReadNR21();
    void WriteNR21(u8 val);

    u8 ReadNR22();
    void WriteNR22(u8 val);

    u8 ReadNR23();
    void WriteNR23(u8 val);

    u8 ReadNR24();
    void WriteNR24(u8 val);

    u8 ReadNR30();
    void WriteNR30(u8 val);

    u8 ReadNR31();
    void WriteNR31(u8 val);

    u8 ReadNR32();
    void WriteNR32(u8 val);

    u8 ReadNR33();
    void WriteNR33(u8 val);

    u8 ReadNR34();
    void WriteNR34(u8 val);

    u8 ReadNR41();
    void WriteNR41(u8 val);

    u8 ReadNR42();
    void WriteNR42(u8 val);

    u8 ReadNR43();
    void WriteNR43(u8 val);

    u8 ReadNR44();
    void WriteNR44(u8 val);

    u8 ReadNR50();
    void WriteNR50(u8 val);

    u8 ReadNR51();
    void WriteNR51(u8 val);

    u8 ReadNR52();
    void WriteNR52(u8 val);

    u8 ReadWaveRAM(u8 addr);
    void WriteWaveRAM(u8 addr, u8 val);

    void Update(unsigned int cycles);

private:
    Hardware& m_hw;
};
