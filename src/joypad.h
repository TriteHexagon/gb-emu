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

const unsigned int joyp_button_a = Bit(0);
const unsigned int joyp_button_b = Bit(1);
const unsigned int joyp_button_select = Bit(2);
const unsigned int joyp_button_start = Bit(3);

const unsigned int joyp_dpad_right = Bit(0);
const unsigned int joyp_dpad_left = Bit(1);
const unsigned int joyp_dpad_up = Bit(2);
const unsigned int joyp_dpad_down = Bit(3);

class Joypad
{
public:
    void Reset();

    u8 ReadJOYP();
    void WriteJOYP(u8 val);

    void SetKeyState(u8 dpad_keys, u8 button_keys);

private:
    bool m_dpad_keys_enable;
    bool m_button_keys_enable;
    u8 m_dpad_keys;
    u8 m_button_keys;
};
