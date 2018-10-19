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
#include "../common.h"
#include "../mapper.h"
#include "../rom.h"

class MBC1 : public Mapper
{
public:
    explicit MBC1(ROMInfo& rom_info);
    virtual void Reset() override;
    virtual u8 Read(u16 addr) override;
    virtual void Write(u16 addr, u8 val) override;

    virtual const std::vector<u8>& GetRAM() override
    {
        return m_ram;
    }

private:
    int GetROMBank();
    int GetRAMBank();
    void UpdateMapping();

    std::vector<u8> m_rom;
    std::vector<u8> m_ram;
    u8* m_rom_map;
    u8* m_ram_map;
    unsigned int m_bank_reg1;
    unsigned int m_bank_reg2;
    bool m_ram_enable;
    bool m_ram_banking_mode;
};
