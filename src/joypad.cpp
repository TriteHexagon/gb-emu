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
#include "joypad.h"

const unsigned int dpad_disable = Bit(4);
const unsigned int button_disable = Bit(5);

void Joypad::Reset()
{
    m_dpad_keys_enable = false;
    m_button_keys_enable = false;
    m_dpad_keys = 0;
    m_button_keys = 0;
}

u8 Joypad::ReadJOYP()
{
    u8 val = 0xFF;

    if (m_dpad_keys_enable)
    {
        val &= ~dpad_disable;
        val &= ~m_dpad_keys;
    }

    if (m_button_keys_enable)
    {
        val &= ~button_disable;
        val &= ~m_button_keys;
    }

    return val;
}

void Joypad::WriteJOYP(u8 val)
{
    m_dpad_keys_enable = ((val & dpad_disable) == 0);
    m_button_keys_enable = ((val & button_disable) == 0);
}

void Joypad::SetKeyState(u8 dpad_keys, u8 button_keys)
{
    m_dpad_keys = dpad_keys;
    m_button_keys = button_keys;
}
