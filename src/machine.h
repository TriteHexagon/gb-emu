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
#include "cpu.h"
#include "memory.h"
#include "timer.h"

class Machine
{
public:
    Machine() :
        m_cpu(*this),
        m_memory(*this),
        m_timer(*this)
    {
    }

    void LoadROM(std::vector<u8> rom);
    void Reset();
    void Run();

    CPU& GetCPU()
    {
        return m_cpu;
    }

    Memory& GetMemory()
    {
        return m_memory;
    }

    Timer& GetTimer()
    {
        return m_timer;
    }

private:
    CPU m_cpu;
    Memory m_memory;
    Timer m_timer;
};
