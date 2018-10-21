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

class MBC3 : public Mapper
{
public:
    explicit MBC3(ROMInfo& rom_info);
    virtual void Reset() override;
    virtual u8 Read(u16 addr) override;
    virtual void Write(u16 addr, u8 val) override;

    virtual const std::vector<u8>& GetRAM() override
    {
        return m_ram;
    }

    virtual std::vector<u8> GetRTCData() override;

private:
    enum RTCReg
    {
        RTC_REG_NONE = 0,
        RTC_REG_S = 8,
        RTC_REG_M = 9,
        RTC_REG_H = 10,
        RTC_REG_DL = 11,
        RTC_REG_DH = 12,
    };

    void UpdateMapping();
    void UpdateRTC();
    void ResetRTCData();

    const bool m_has_rtc;
    std::vector<u8> m_rom;
    std::vector<u8> m_ram;
    u8* m_rom_map;
    u8* m_ram_map;
    unsigned int m_rom_bank;
    unsigned int m_ram_bank;
    bool m_ram_enable;

    RTCReg m_rtc_current_reg;
    int m_rtc_latch_state;
    u8 m_rtc_s;
    u8 m_rtc_m;
    u8 m_rtc_h;
    u8 m_rtc_dl;
    u8 m_rtc_dh;
    u8 m_rtc_latched_s;
    u8 m_rtc_latched_m;
    u8 m_rtc_latched_h;
    u8 m_rtc_latched_dl;
    u8 m_rtc_latched_dh;
    s64 m_rtc_last_update;
};
