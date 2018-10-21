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
#include "machine.h"

Machine::Machine(ROMInfo& rom_info) : m_hw(rom_info.is_cgb_aware)
{
    m_hw.memory.LoadROM(rom_info);
    Reset();
}

void Machine::Reset()
{
    m_hw.cpu.Reset();
    m_hw.memory.Reset();
    m_hw.timer.Reset();
    m_hw.audio.Reset();
    m_hw.graphics.Reset();
    m_hw.joypad.Reset();
}

void Machine::Run(unsigned int cycles)
{
    m_hw.cpu.Run(cycles);
}

void Machine::SetKeyState(u8 dpad_keys, u8 button_keys)
{
    m_hw.joypad.SetKeyState(dpad_keys, button_keys);
}

void Machine::SetTraceLogEnabled(bool enabled)
{
    m_hw.cpu.SetTraceLogEnabled(enabled);
}
