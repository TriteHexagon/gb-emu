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

class Timer
{
public:
    explicit Timer(Hardware& hw) : m_hw(hw)
    {
    }

    void Reset();

    u8 ReadDIV();
    void WriteDIV();

    u8 ReadTIMA();
    void WriteTIMA(u8 val);

    u8 ReadTMA();
    void WriteTMA(u8 val);

    u8 ReadTAC();
    void WriteTAC(u8 val);

    void Update(unsigned int cycles);

private:
    int CalcSelectedBitValue();

    Hardware& m_hw;

    u16 m_counter;
    int m_selected_bit_index;
    int m_selected_bit_value;

    u8 m_reg_tima;
    u8 m_reg_tma;
    unsigned int m_timer_clock_select;
    bool m_timer_enable;
};
