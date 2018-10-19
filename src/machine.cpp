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

void Machine::LoadROM(ROMInfo& rom_info)
{
    m_devices.memory.LoadROM(rom_info);
}

void Machine::Reset()
{
    m_devices.cpu.Reset();
    m_devices.memory.Reset();
    m_devices.timer.Reset();
    m_devices.graphics.Reset();
    m_devices.joypad.Reset();
}

void Machine::Run(unsigned int cycles)
{
    m_devices.cpu.Run(cycles);
}

void Machine::SetKeyState(u8 dpad_keys, u8 button_keys)
{
    m_devices.joypad.SetKeyState(dpad_keys, button_keys);
}
