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

#include "common.h"
#include "timer.h"
#include "machine.h"

const unsigned int clock_cycles_per_machine_cycle = 4;

const unsigned int tac_clock_select = 0x3;
const unsigned int tac_enable = Bit(2);

// The timer's selected bit index depends on bits 1:0 of TAC.
// The resulting clock rate is (4194304 Hz >> (bit_index + 1)).
// 00:   4096 Hz (9)
// 01: 262144 Hz (3)
// 10:  65536 Hz (5)
// 11:  16384 Hz (7)
const int bit_index_table[] = { 9, 3, 5, 7 };

int Timer::CalcSelectedBitValue()
{
    return (m_counter & Bit(m_selected_bit_index)) ? 1 : 0;
}

void Timer::Reset()
{
    m_counter = 0;
    m_selected_bit_index = bit_index_table[0];
    m_selected_bit_value = 0;

    m_reg_tima = 0;
    m_reg_tma = 0;
    m_timer_clock_select = 0;
    m_timer_enable = 0;
}

u8 Timer::ReadDIV()
{
    return m_counter >> 8;
}

void Timer::WriteDIV()
{
    m_counter = 0;
}

u8 Timer::ReadTIMA()
{
    return m_reg_tima;
}

void Timer::WriteTIMA(u8 val)
{
    m_reg_tima = val;
}

u8 Timer::ReadTMA()
{
    return m_reg_tma;
}

void Timer::WriteTMA(u8 val)
{
    m_reg_tma = val;
}

u8 Timer::ReadTAC()
{
    return (m_timer_enable ? tac_enable : 0) | m_timer_clock_select;
}

// TODO: Implement the hardware glitch that can occur when writing to this register.
void Timer::WriteTAC(u8 val)
{
    m_timer_enable = ((val & tac_enable) != 0);
    m_timer_clock_select = val & tac_clock_select;
    m_selected_bit_index = bit_index_table[m_timer_clock_select];
    m_selected_bit_value = CalcSelectedBitValue();
}

void Timer::Update(unsigned int cycles)
{
    m_counter += cycles * clock_cycles_per_machine_cycle;

    if (m_timer_enable)
    {
        int old_value = m_selected_bit_value;
        m_selected_bit_value = CalcSelectedBitValue();

        if (old_value == 1 && m_selected_bit_value == 0)
        {
            if (m_reg_tima == 0xFF)
            {
                m_reg_tima = m_reg_tma;
                m_hw.cpu.SetInterruptFlag(intr_timer);
            }
            else
            {
                m_reg_tima++;
            }
        }
    }
}
